# C++ Stride Compiler

This project implements a basic compiler for the Stride language, as defined in language.md.

## Structure
- `include/`: Header files for tokens, tokenizer, parser, AST nodes
- `src/`: Source files for implementation
- `language.md`: BNF grammar

## Build
Use CMake to build the project:

```sh
mkdir build && cd build
cmake ..
make
```

## Next Steps
- Implement tokenizer logic in `src/tokenizer.cpp`
- Expand AST node types in `include/ast_node.h`
- Implement parser logic in `src/parser.cpp`
- Add error handling and unit tests

