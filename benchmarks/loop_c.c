#include <stdio.h>
#include <stdint.h>

int main() {
    int32_t i = 0;
    int32_t sum = 0;
    while (i < 2000000000) {
        sum = sum + 1;
        i = i + 1;
    }
    printf("%d\n", sum);
    return 0;
}
