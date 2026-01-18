#include <stdio.h>
#include <stdint.h>

int32_t f1(int32_t x) { return x + 1; }
int32_t f2(int32_t x) { return f1(x) * 2; }
int32_t f3(int32_t x) { return f2(x) - 1; }
int32_t f4(int32_t x) { return f3(x) + f1(x); }
int32_t f5(int32_t x) { return f4(x) + f2(x); }
int32_t f6(int32_t x) { return f5(x) - f3(x); }

int main() {
    int32_t i = 0;
    int32_t sum = 0;
    while (i < 500000000) {
        sum = sum + f6(i % 10);
        i = i + 1;
    }
    printf("%d\n", sum);
    return 0;
}
