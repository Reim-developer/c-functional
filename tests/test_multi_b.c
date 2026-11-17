/*
 * Translation unit B of the multi-TU test: main, calling shared_sum
 * from the other unit and using the same combinators itself.
 */

#include <stdio.h>

#include "shared_int.h"

static int is_even(int v, void *ctx) {
    (void)ctx;
    return v % 2 == 0;
}

int main(void) {
    int data[] = {1, 2, 3, 4, 5};
    int buf[5];
    int total;
    shared_i32_slice evens;
    int failed = 0;

    total = shared_sum(data, 5);
    if (total != 15) {
        fprintf(stderr, "FAIL: shared_sum = %d, want 15\n", total);
        failed = 1;
    }

    evens = shared_i32_filter(shared_i32_from(data, 5), buf,
        is_even, NULL);
    if (evens.len != 2 || buf[0] != 2 || buf[1] != 4) {
        fprintf(stderr, "FAIL: filter across TUs gave len %lu\n",
            (unsigned long)evens.len);
        failed = 1;
    }

    if (failed == 0) {
        printf("test_multi: passed\n");
    }
    return failed;
}
