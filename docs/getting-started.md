# Getting Started

Follow these instructions to build and run the Stride compiler and editor.

## Prerequisites

1. CMake >= 3.1
2. Node.js >= 24.0
3. LLVM 21.1.8

### Installation

You can install these packages using the following commands:

```shell
# MacOS 
brew install cmake node@24 llvm@21
```

```shell
# Ubuntu/Debian
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
./cmake-build-debug/cstride <file_path>
```

Or if you added it to your PATH:
```sh
cstride <file_path>
```

## Running the Editor

The web-based editor provides a convenient way to write Stride code with syntax highlighting and integrated terminal.

### Prerequisites for Editor
- `node` (>= 24.0)
- `pnpm` (Performant Node Package Manager)

### Local Setup and Running

To run the editor locally on your machine:

1. **Navigate to the editor directory**:
   ```sh
   cd packages/editor
   ```

2. **Install dependencies**:
   Using `pnpm` ensures fast and efficient dependency management.
   ```sh
   pnpm install
   ```

3. **Start the development server**:
   Launch the editor in development mode.
   ```sh
   pnpm start
   ```

4. **Access the Editor**:
   Once started, the editor will be available at `http://localhost:5173`. Open this URL in your web browser.

### Features
- **Real-time Syntax Highlighting**: Visualizes Stride code structure.
- **Integrated Terminal**: Run your Stride programs directly within the editor environment.
- **Local Development**: Test changes to the editor itself or use it as your primary development environment for Stride.
