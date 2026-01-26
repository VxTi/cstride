# Stride Language Compiler and Editor

This project implements a basic compiler for the Stride programming language, along with a web-based code editor featuring syntax highlighting.

## Project Structure

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

## Building the Compiler

1. Navigate to the compiler directory:
   ```sh
   cd packages/compiler
   ```
2. Build using CMake:
   ```sh
   cmake -S . -B cmake-build-debug
   cmake --build cmake-build-debug --target cstride
   ```

## Running the Compiler

From the `packages/compiler` directory, you can either run it directly:
```sh
./cmake-build-debug/cstride <file1> <file2> ...
```
Or move the executable to a directory in your PATH and run:
```sh
cstride <file1> <file2> ...
```

Replace `<file>` with your Stride source file.

## Using the Editor

The editor is a web-based IDE for Stride, built with Monaco Editor.

1. Navigate to the editor directory and install dependencies:
   ```sh
   cd packages/editor
   npm install
   ```
2. Start the development server:
   ```sh
   npm start
   ```
3. Open the provided local URL in your browser.

## Standard Library

Stride's standard library is located in `packages/standard-library/` and includes math, memory, and I/O modules.

## Contributing

Contributions are welcome! Please open issues or submit pull requests for improvements or bug fixes.
