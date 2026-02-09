## Stride Language Compiler and Editor

This project implements a compiler for the Stride programming language,
along with a web-based code editor featuring syntax highlighting.

Stride is a statically typed, JIT-compiled language using the LLVM compiler toolchain.
Currently, it only supports primitive types, though support for structs, classes, and generics is planned.

Structs are already partially implemented, though member access is not yet implemented.

### Project Structure

```
cstride/
└── packages/
    ├── compiler/
    │   └── ... (compiler source files)
    ├── standard-library/
    │   └── ... (standard library files)
    └── stride-editor/
        └── ... (web-based editor files)  
```

### Prerequisites

1. CMake >= 3.1
2. Node.js >= 20.0 (LTS)
3. LLVM (tested with 17, 18)
4. GoogleTest

You can install these packages using the following commands:

**macOS (using Homebrew):**
```shell
brew install cmake node@22 llvm@21 googletest
```

**Ubuntu/Debian:**
```shell
sudo apt install cmake nodejs llvm-21 googletest
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

### Using the Editor

<img width="678" height="641" alt="Screenshot 2026-01-29 at 17 21 54" src="https://github.com/user-attachments/assets/ade3bee2-4bcb-44d9-8c61-04d5cbc99ff1" />

The editor is a web-based IDE for Stride, built with Monaco Editor.

1. Navigate to the editor directory and install dependencies:
   ```sh
   cd packages/stride-editor
   npm install
   ```
2. Start the development server:
   ```sh
   npm start
   ```
3. Open the provided local URL in your browser.
   This will likely be `http://localhost:3000/`

## Language Reference

### Syntax

### Creating variables

```stride
// Creating a mutable 32 bit int variable
let x: i32 = 10

// Creating a constant variable
const y: f64 = 3.0D

// There are a few different kinds of integer literals, like so:
let hex: i32 = 0x12345

// 64-bit integer literal, suffixed with an 'L'
let large_int: i64 = 12345678901234567890L
```

### Integer Literals

Stride supports various integer literal formats, including decimal, hexadecimal, and binary.
For example, `0x12345` is a hexadecimal integer literal, and `0b101010` is a binary integer literal.

### Floating Point Literals

Floating point literals can be suffixed with 'F' for single precision or 'D' for double precision.
For example, `3.14F` is a single precision float, and `2.71828D` is a double precision float.

### Operators

The language uses the traditional arithmetic operators, as well as comparison operators.
An example of these operators is shown below:
```stride
let x: i32 = 10
let y: f64 = 3.0D

// Arithmetic operators
let sum: f64 = x + y
let difference: f64 = x - y
let product: f64 = x * y
let quotient: f64 = x / y
let remainder: f64 = x % y

// Comparison operators
let is_equal: bool = x == y
let is_not_equal: bool = x != y
let is_less_than: bool = x < y
let is_greater_than: bool = x > y
let is_less_than_or_equal: bool = x <= y
let is_greater_than_or_equal: bool = x >= y
```

### Creating and invoking functions 

Functions are declared using the `fn` keyword, followed by the function name,
a list of parameters (optionally), and the return type. For example:
```stride
fn add(a: i32, b: i32): i32 {
    return a + b
}

// You can also create externally linked functions,
// if they are defined in another library.
extern fn some_c_function(x: i32): i32;


// You can invoke functions using the following syntax:
const result: i32 = add(10, 5)
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

### Structs

Structs are still in development, but they are similar to classes in other languages.
The current syntax is as follows:
```stride
struct Point {
    x: i32,
    y: i32,
}

const p = Point::{ x: 10, y: 20 }

// You can also create structs that reference other structs.
// These still function as separate types, and will cause a type error if they are used inappropriately.
struct Vector2d = Point

const p2: Vector2d = Vector2d::{ x: 5, y: 15 } // This is valid

const p3: Point = p2 // This is invalid,
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
