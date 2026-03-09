---
layout: home

hero:
  name: Stride Language
  text: A High-Performance, Statically Typed Systems Language.
  tagline: Stride is a multi-paradigm, JIT-compiled systems programming language built on the LLVM compiler toolchain, combining low-level control with modern syntax.
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
type Point = { x: i32; y: i32; };

fn main(): void {
    const p: Point = Point::{ x: 10, y: 20 };
    let list: i32[] = [1, 2, 3];
    let maybe_val: i32? = nil;

    if (p.x > 5) {
        maybe_val = p.x + list[0];
    }

    const op: (i32, i32) -> i32 = (a: i32, b: i32): i32 -> { return a + b; };
    printf("Result: %d\n", op(p.x, p.y));
}
```

## Project Status

Stride is currently in active development. Core language features like primitive types, functions, and control flow are stable. More advanced features like full struct support, imports, and a comprehensive standard library are currently being implemented.
