#include <stdio.h>
#include <stdint.h>

int main() {
    int32_t i = 0;
    int32_t count = 0;
    while (i < 40000) {
        int32_t j = 0;
        while (j < 40000) {
            count = count + 1;
            j = j + 1;
        }
        i = i + 1;
    }
    printf("%d\n", count);
    return 0;
}
