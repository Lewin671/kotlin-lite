#include <stdio.h>
#include <stdint.h>

int32_t is_prime(int32_t n) {
    if (n <= 1) return 0;
    int32_t i = 2;
    while (i * i <= n) {
        if (n % i == 0) return 0;
        i = i + 1;
    }
    return 1;
}

int main() {
    int32_t count = 0;
    int32_t n = 2;
    while (n < 500000) {
        if (is_prime(n) == 1) {
            count = count + 1;
        }
        n = n + 1;
    }
    printf("%d\n", count);
    return 0;
}

