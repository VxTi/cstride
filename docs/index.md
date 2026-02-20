---
layout: home

hero:
  name: Stride Language
  text: Stride is a statically typed, JIT-compiled language built on the LLVM compiler toolchain.
  tagline: Combine the performance and control of low-level languages with the simplicity and safety of modern syntax.
  actions:
    - theme: brand
      text: Getting Started
      link: /getting-started
    - theme: alt
      text: View on GitHub
      link: https://github.com/vxti/cstride

features:
  - title: High Performance
    details: Leverages LLVM for optimized machine code generation.
  - title: Static Typing
    details: Catch errors early with a robust type system.
  - title: JIT Compilation
    details: Execute code instantly without a separate compilation step.
---

## Key Features

- **Modern Syntax**: Clean and expressive syntax inspired by C, Rust, and TypeScript.
- **Integrated Tooling**: Comes with a dedicated local web editor for an immediate development experience.

## A Taste of Stride

```stride
fn main(): void {
    let limit: int32 = 10
    printf("Counting to %d:\n", limit)
    
    for (let i: int32 = 1; i <= limit; i++) {
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
