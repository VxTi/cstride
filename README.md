## Stride Language Compiler

Stride is a statically typed, multi-paradigm, JIT-compiled systems programming language designed for high performance, safety, and developer productivity. Built on the LLVM compiler toolchain, Stride combines the efficiency of low-level languages like C++ with the modern, expressive syntax found in languages like TypeScript and Rust.

Stride is tailored for performance-critical applications, systems programming, and high-performance computing, providing robust type safety, predictable memory management, and seamless integration with existing libraries.

### Project Structure

```
cstride/
└── packages/
    ├── compiler/                   # Stride compiler (C++, LLVM)
    ├── standard-library/           # Stride standard library (.sr)
    ├── stride-plugin-intellij/     # IntelliJ IDEA plugin (Kotlin)
    └── stride-plugin-vscode/       # Visual Studio Code extension (TypeScript)
```

### Prerequisites

1. **LLVM** (21+ - developed with 21.1.8)
2. **CMake** >= 3.1
3. **GoogleTest** (for compiler tests)
4. **Java JDK** >= 17 (for IntelliJ plugin development)
5. **Node.js** (for VSCode extension development)

**macOS (using Homebrew):**
```shell
brew install cmake llvm@21 googletest openjdk@17 node
```

**Ubuntu/Debian:**
```shell
sudo apt install cmake llvm-21-dev libgtest-dev openjdk-17-jdk nodejs
```

### Getting started

1. Install prerequisites as described above.
2. Clone the repository:
   ```sh
   git clone https://github.com/VxTi/cstride.git
   cd cstride
   ```
   The `packages/` directory contains the compiler and editor source code.
3. Follow the instructions below to build and run the compiler and editor.

### Building the Compiler

Run the following commands from the project root directory:

```sh
cd packages/compiler
mkdir cmake-build-debug
cd cmake-build-debug
cmake ..
make
```

*Optionally*: Add the binary to your `$PATH`, if you wish to use it directly.

### Running the Compiler

From the `packages/compiler` directory, you can either run it directly:
```sh
cstride -c <file1> <file2> ...
```
Replace `<file>` with your Stride source file.

*Note: This is assuming you have the binary in your `$PATH`, or are in the build directory.*

For additional commands, run `cstride --help`.
This CLI tool is also still in development.

### Running Tests

To run the compiler tests, run the following commands from `packages/compiler/cmake-build-debug`:

```sh
make cstride_tests
./tests/cstride_tests
```

## Language Reference

### Types and Variables

Stride is statically typed and supports various primitive and complex types.

```stride
// Primitive types
let a: i32 = 10;
let b: i64 = 100L;
let c: f32 = 3.14F;
let d: f64 = 2.718D;
let e: bool = true;
let f: char = 'A';
let g: string = "Hello, Stride!";

// Optional types (can be nil)
let h: i32? = nil;
h = 42;

// Arrays (fixed-size or dynamic-size)
let i: i32[] = [1, 2, 3, 4, 5];
let j: f64[][] = [[1.0, 2.0], [3.0, 4.0]];

// Type Aliases
type IntList = i32[];
let k: IntList = [10, 20, 30];
```

### Structs

Structs allow grouping related data. They use nominal typing, meaning each type definition is unique.

```stride
type Point = {
    x: i32;
    y: i32;
};

// Initialization using the ::{ } syntax
const p: Point = Point::{ x: 10, y: 20 };

// Composition
type Rect = {
    origin: Point;
    width: i32;
    height: i32;
};

const r = Rect::{
    origin: Point::{ x: 0, y: 0 },
    width: 100,
    height: 100
};
```

### Functions and Lambdas

Functions are first-class citizens in Stride.

```stride
// Standard function declaration
fn add(a: i32, b: i32): i32 {
    return a + b;
}

// Function types and references
const op: (i32, i32) -> i32 = add;
let result = op(10, 20);

// Extern functions (linking to C)
extern fn printf(format: string, ...): i32;
```

### Operators

Stride supports standard arithmetic and comparison operators with dominant type promotion.

```stride
let x = 10 + 20L; // x is i64
let y = 10.0 + 5; // y is f64

let is_equal = (x == 30L);
```

### Local Documentation Preview

You can preview the documentation locally using MkDocs:

```sh
pip install mkdocs-material
mkdocs serve
```

### Comments

Comments in Stride are denoted by `//` for single-line comments, and `/* ... */` for multi-line comments.
```stride
// This is a single-line comment
/*
This is a multi-line comment
*/
```


### Imports

Imports are currently not supported, though they are resolved during compilation.

The format for imports is as follows:
```stride
import <package>::{ Module1, Module2, ... };


// Let's say we're trying to import math functions from the standard library.
import std::{ Math };
```

## Standard Library

Stride's standard library is located in `packages/standard-library/` and includes math, memory, and I/O modules.
This is still in development.

## Contributing

Contributions are welcome! Please open issues or submit pull requests for improvements or bug fixes.
