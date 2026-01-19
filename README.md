# C++ Stride Compiler

This project implements a basic compiler for the Stride language

## Structure
- `include/`: Header files for tokens, tokenizer, parser, AST nodes
- `src/`: Source files for implementation
- `editor`: A Monaco based editor with code highlighting for the language

## Build

Use CMake to build the project:

```sh
cmake --build cmake-build-debug --target cstride
```

## Usage

You can start using the stride compiler by running:
```shell
./cmake-build-debug/cstride <file>
```
(Assuming you are in the build directory)

To make coding easier, you can use the editor in the `editor` directory.
This you can start by running the command

```shell
npm run dev
```


