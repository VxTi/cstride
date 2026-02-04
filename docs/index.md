# Stride Language

Stride is a statically typed, JIT-compiled language using the LLVM compiler toolchain.

This project implements a compiler for the Stride programming language, along with a web-based code editor featuring syntax highlighting.

Currently, it only supports primitive types, though support for structs, classes, and generics is planned.

Structs are already partially implemented, though member access is not yet implemented.

## Project Structure

```
cstride/
└── packages/
    ├── compiler/
    │   └── ... (compiler source files)
    ├── standard-library/
    │   └── ... (standard library files)
    └── editor/
        └── ... (web-based editor files)  
```
