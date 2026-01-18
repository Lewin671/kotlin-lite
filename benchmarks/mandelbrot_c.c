#include <stdio.h>
#include <stdint.h>

int main() {
    int32_t count = 0;
    int32_t y = -1500;
    while (y < 1500) {
        int32_t x = -2000;
        while (x < 1000) {
            int32_t cr = x;
            int32_t ci = y;
            int32_t zr = 0;
            int32_t zi = 0;
            int32_t iter = 0;
            int32_t escaped = 0;
            while (iter < 100) {
                int32_t zr2 = (zr * zr) / 1000;
                int32_t zi2 = (zi * zi) / 1000;
                if (zr2 + zi2 > 4000) {
                    escaped = 1;
                    iter = 100;
                } else {
                    zi = (2 * zr * zi) / 1000 + ci;
                    zr = zr2 - zi2 + cr;
                    iter = iter + 1;
                }
            }
            if (escaped == 0) {
                count = count + 1;
            }
            x = x + 20;
        }
        y = y + 20;
    }
    printf("%d\n", count);
    return 0;
}
