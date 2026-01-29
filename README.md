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
    └── editor/
        └── ... (web-based editorfiles)  
```

### Prerequisites

1. CMake >= 3.1
2. Node.js >= 24.0
3. LLVM 21.1.8

You can install these packages using the following commands:

```shell
brew install cmake node@24 llvm@21 # macOS using Homebrew
sudo apt install cmake nodejs llvm-21 # Ubuntu/Debian
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
   cd packages/compiler;
   cmake -S . -B cmake-build-debug &&
   cmake --build cmake-build-debug --target cstride
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

### Using the Editor

The editor is a web-based IDE for Stride, built with Monaco Editor.

1. Navigate to the editor directory and install dependencies:
   ```sh
   cd packages/editor;
   npm install
   ```
2. Start the development server:
   ```sh
   npm start
   ```
3. Open the provided local URL in your browser.
   This will likely be `http://localhost:3000/`

## Standard Library

Stride's standard library is located in `packages/standard-library/` and includes math, memory, and I/O modules.
This is still in development.

## Contributing

Contributions are welcome! Please open issues or submit pull requests for improvements or bug fixes.
