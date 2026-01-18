#!/bin/bash

# Build the compiler first
mkdir -p build && cd build && cmake .. > /dev/null && make -j$(sysctl -n hw.ncpu) > /dev/null || exit 1
cd ..

KOTLIN_LITE="./build/kotlin-lite"
KOTLINC="kotlinc"
KOTLIN="kotlin"

BENCHMARKS=("fib.kt" "loop.kt" "prime.kt" "nested.kt")

echo "------------------------------------------------------------"
echo "| Benchmark | kotlin-lite (s) | kotlinc (s) | Speedup |"
echo "------------------------------------------------------------"

for b in "${BENCHMARKS[@]}"; do
    FILE="benchmarks/$b"
    
    # Compile and run with kotlin-lite
    $KOTLIN_LITE "$FILE" -o "bench_lite" > /dev/null
    START=$(python3 -c 'import time; print(time.time())')
    ./bench_lite > /dev/null
    END=$(python3 -c 'import time; print(time.time())')
    LITE_TIME=$(python3 -c "print(f'{$END - $START:.3f}')")
    
    # Compile and run with kotlinc
    $KOTLINC "$FILE" benchmarks/compat.kt -include-runtime -d "bench_jvm.jar"
    START=$(python3 -c 'import time; print(time.time())')
    java -jar "bench_jvm.jar" > /dev/null
    END=$(python3 -c 'import time; print(time.time())')
    JVM_TIME=$(python3 -c "print(f'{$END - $START:.3f}')")
    
    SPEEDUP=$(python3 -c "print(f'{float($JVM_TIME)/float($LITE_TIME):.2f}')" 2>/dev/null || echo "N/A")
    
    printf "| %-9s | %-15s | %-11s | %-7s |\n" "$b" "$LITE_TIME" "$JVM_TIME" "$SPEEDUP"
done

echo "------------------------------------------------------------"

# Cleanup
rm -f bench_lite bench_jvm.jar output.ll program
