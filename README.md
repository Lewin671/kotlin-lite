# Kotlin Lite Compiler

A minimal Kotlin subset compiler with SSA-based IR and LLVM code generation.

## Quick Start

### Prerequisites
- C++17+, LLVM 14+, CMake 3.10+

### Building
```bash
mkdir build && cd build
cmake ..
make
```

### Running
```bash
./kotlin-lite <input.kt>  # Generates a.out
./a.out                    # Execute compiled program
```

## Supported Features

- **Types:** Int, Boolean, Unit
- **Functions:** Top-level definitions with returns
- **Variables:** `val` (immutable) and `var` (mutable)
- **Control Flow:** if/else, while loops, break, continue
- **Operators:** Arithmetic (+, -, *, /, %), comparison (==, !=, <, >, <=, >=), logical (!, &&, ||)
- **Built-ins:** `print_i32()`, `print_bool()`

## Example

```kotlin
fun factorial(n: Int): Int {
    if (n <= 1) {
        return 1
    } else {
        return n * factorial(n - 1)
    }
}

fun main(): Unit {
    print_i32(factorial(5))
}
```

## Architecture

The compiler pipeline:
1. **Lexer/Parser** → AST
2. **Semantic Analysis** → Type checking, symbol resolution
3. **IR Generation** → Custom SSA-CFG IR
4. **LLVM Codegen** → LLVM IR → Machine code

Key design: Environment-based phi insertion for control flow merging, eliminating memory operations at the IR level.

See [Architecture Guide](docs/architecture.md) for details.

## Development

Milestones progress from basic functions → variables → conditionals → loops → short-circuit evaluation.

To add features:
1. Extend lexer/parser
2. Update AST
3. Implement semantic analysis
4. Add IR lowering
5. Write tests

See [Architecture Guide](docs/architecture.md) for detailed implementation notes.

## Benchmarks

Performance comparison between `kotlin-lite` and standard `kotlinc` is available in the `benchmarks/` directory.

To run the benchmarks:
```bash
./benchmarks/run_benchmarks.sh
```

See the [Benchmarks Guide](docs/benchmarks.md) for more details.

## License

This project is licensed under the [MIT License](LICENSE).
