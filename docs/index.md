# Stride Language

Stride is a statically typed, JIT-compiled language built on the LLVM compiler toolchain. It aims to combine the performance and control of low-level languages with the simplicity and safety of modern syntax.

This project includes a complete compiler for the Stride language and a modern web-based code editor with syntax highlighting and an integrated terminal.

## Key Features

- **High Performance**: Leverages LLVM for optimized machine code generation.
- **Static Typing**: Catch errors early with a robust type system.
- **JIT Compilation**: Execute code instantly without a separate compilation step.
- **Modern Syntax**: Clean and expressive syntax inspired by C, Rust, and TypeScript.
- **Integrated Tooling**: Comes with a dedicated local web editor for an immediate development experience.

## Quick Start

1.  **[Getting Started](getting-started.md)**: Learn how to install the compiler and set up the local editor.
2.  **[Examples](examples.md)**: Explore various code samples, from "Hello World" to Taylor series approximations.
3.  **[Language Reference](reference/variables.md)**: Dive deep into Stride's syntax, types, and features.

## A Taste of Stride

```stride
fn main(): void {
    let limit: i32 = 10
    printf("Counting to %d:\n", limit)
    
    for (mut i: i32 = 1; i <= limit; i++) {
        if (i % 2 == 0) {
            printf("%d is even\n", i)
        } else {
            printf("%d is odd\n", i)
        }
    }
}
```

## Project Status

Stride is currently in active development. Core language features like primitive types, functions, and control flow are stable. More advanced features like full struct support, imports, and a comprehensive standard library are currently being implemented.
