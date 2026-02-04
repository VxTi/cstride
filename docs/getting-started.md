# Getting Started

Follow these instructions to build and run the Stride compiler and editor.

## Prerequisites

1. CMake >= 3.1
2. Node.js >= 24.0
3. LLVM 21.1.8

### Installation

You can install these packages using the following commands:

=== "macOS"

    ```shell
    brew install cmake node@24 llvm@21
    ```

=== "Ubuntu/Debian"

    ```shell
    sudo apt install cmake nodejs llvm-21
    ```

## Installation

1. Clone the repository:
   ```sh
   git clone https://github.com/VxTi/cstride.git
   cd cstride
   ```
   The `packages/` directory contains the compiler and editor source code.

## Building the Compiler

Run the following commands from the project root directory:
```sh
cd packages/compiler
cmake -S . -B cmake-build-debug
cmake --build cmake-build-debug --target cstride
```

*Optionally*: Add the binary to your `$PATH`, if you wish to use it directly.

## Running the Compiler

From the `packages/compiler` directory, you can either run it directly:
```sh
cstride -c <file1> <file2> ...
```
Replace `<file>` with your Stride source file.

*Note: This assumes you have the binary in your `$PATH`, or are in the build directory.*

For additional commands, run `cstride --help`.

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
3. Open the provided local URL in your browser (likely `http://localhost:3000/`).
