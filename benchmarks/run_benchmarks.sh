#!/bin/bash

# Build the compiler first
mkdir -p build && cd build && cmake .. > /dev/null && make -j$(sysctl -n hw.ncpu) > /dev/null || exit 1
cd ..

KOTLIN_LITE="./build/kotlin-lite"

KOTLINC="kotlinc"



# Create a temporary directory for benchmark files

TMP_DIR=$(mktemp -d)

trap 'rm -rf "$TMP_DIR"' EXIT



BENCHMARKS=("fib.kt" "loop.kt" "prime.kt" "nested.kt" "mandelbrot.kt" "inline.kt")



echo "----------------------------------------------------------------------------"

echo "| Benchmark     | Lite (s) | JVM (s)  | C (s)    | Lite/C | Lite/JVM |"

echo "----------------------------------------------------------------------------"



for b in "${BENCHMARKS[@]}"; do

    FILE="benchmarks/$b"

    BASENAME="${b%.kt}"

    

    # 1. kotlin-lite

    $KOTLIN_LITE "$FILE" -o "$TMP_DIR/bench_lite" > /dev/null

    START=$(python3 -c 'import time; print(time.time())')

    "$TMP_DIR/bench_lite" > /dev/null

    END=$(python3 -c 'import time; print(time.time())')

    LITE_TIME=$(python3 -c "print(f'{$END - $START:.3f}')")

    

    # 2. kotlinc (JVM) with warmup

    $KOTLINC "$FILE" benchmarks/compat.kt -include-runtime -d "$TMP_DIR/bench_jvm.jar"

    java -jar "$TMP_DIR/bench_jvm.jar" > /dev/null # warmup

    START=$(python3 -c 'import time; print(time.time())')

    java -jar "$TMP_DIR/bench_jvm.jar" > /dev/null

    END=$(python3 -c 'import time; print(time.time())')

    JVM_TIME=$(python3 -c "print(f'{$END - $START:.3f}')")

    

    # 3. Pure C (Clang -O3)

    C_FILE="benchmarks/${BASENAME}_c.c"

    clang -O3 "$C_FILE" -o "$TMP_DIR/bench_c"

    START=$(python3 -c 'import time; print(time.time())')

    "$TMP_DIR/bench_c" > /dev/null

    END=$(python3 -c 'import time; print(time.time())')

    C_TIME=$(python3 -c "print(f'{$END - $START:.3f}')")

    

    LITE_C_RATIO=$(python3 -c "print(f'{float($LITE_TIME)/float($C_TIME):.2f}')" 2>/dev/null || echo "N/A")

    LITE_JVM_RATIO=$(python3 -c "print(f'{float($LITE_TIME)/float($JVM_TIME):.2f}')" 2>/dev/null || echo "N/A")

    

    printf "| %-13s | %-8s | %-8s | %-8s | %-6s | %-8s |\n" "$b" "$LITE_TIME" "$JVM_TIME" "$C_TIME" "$LITE_C_RATIO" "$LITE_JVM_RATIO"

done



echo "----------------------------------------------------------------------------"
