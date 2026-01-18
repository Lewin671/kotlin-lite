#include <stdio.h>
#include <stdint.h>

void print_i32(int32_t value) {
    printf("%d\n", value);
}

void print_bool(int8_t value) {
    if (value) {
        printf("true\n");
    } else {
        printf("false\n");
    }
}

