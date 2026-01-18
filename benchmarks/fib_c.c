#include <stdio.h>
#include <stdint.h>

int32_t fib(int32_t n) {
    if (n <= 1) {
        return n;
    }
    return fib(n - 1) + fib(n - 2);
}

int main() {
    printf("%d\n", fib(42));
    return 0;
}

