# Benchmarks

This document describes the benchmarking suite used to evaluate the performance of `kotlin-lite` compared to the standard Kotlin compiler (`kotlinc` JVM).

## Overview

The benchmarking suite consists of several Kotlin programs that test different aspects of the compiler's performance, particularly loop optimizations and recursion.

### Benchmark Cases

1.  **`fib.kt`**: Calculates the 42nd Fibonacci number recursively. This tests function call overhead and recursion performance.
2.  **`loop.kt`**: A tight loop that increments a counter 2 billion times. This tests basic loop performance and induction variable optimization.
3.  **`prime.kt`**: Counts prime numbers up to 500,000 using a trial division algorithm. This tests nested loops and modulo operations.
4.  **`nested.kt`**: A nested loop (40,000 x 40,000) that increments a counter. This tests nested control flow and phi node merging.

## Running Benchmarks

A script is provided to automate the building of the compiler, compilation of benchmarks, and execution timing.

### Prerequisites

-   `kotlinc` (Kotlin command-line compiler)
-   `java` (Java Runtime Environment)
-   `python3` (used for high-precision timing)
-   `clang` (used by `kotlin-lite` for linking)

### Execution

From the project root, run:

```bash
./benchmarks/run_benchmarks.sh
```

The script will:
1.  Build the `kotlin-lite` compiler.
2.  Compile each benchmark with `kotlin-lite` (using `-O3` via `clang`).
3.  Compile each benchmark with `kotlinc` into a JVM JAR.
4.  Measure the execution time of both versions.
5.  Report the results in a table including the calculated speedup.

## Methodology

### kotlin-lite
The `kotlin-lite` compiler generates LLVM IR from the Kotlin source. It then invokes `clang` with `-O3` to perform aggressive optimizations and generate a native binary. This allows us to benefit from LLVM's mature optimization passes (loop unrolling, vectorization, etc.).

### kotlinc (JVM)
The standard Kotlin compiler targets the JVM. While the bytecode itself isn't heavily optimized, the HotSpot JIT compiler performs sophisticated optimizations at runtime. The benchmark includes the JVM startup time, which may impact results for shorter-running benchmarks.

## Performance Analysis

In many cases, `kotlin-lite` (via LLVM `-O3`) can outperform the JVM for tight, compute-bound loops due to:
-   Zero-cost abstraction for local variables (SSA form).
-   Direct native code execution without JIT overhead.
-   Aggressive LLVM loop optimizations.

However, for very short tasks, JVM startup time is the dominant factor, while for long-running complex tasks, the JIT might reach similar performance levels to native code.
