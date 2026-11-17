/*
 * Quickstart example from the README. Kept as a real program so the
 * README output can never drift from what the code actually prints.
 * Build and run with: make test
 */

#include <stdio.h>

#include "cf.h"

CF_DEFINE(int, i32);

static int is_even(int v, void *ctx) {
    (void)ctx;
    return v % 2 == 0;
}

static int square(int v, void *ctx) {
    (void)ctx;
    return v * v;
}

static int add(int acc, int v, void *ctx) {
    (void)ctx;
    return acc + v;
}

static void print_slice(const char *label, i32_slice s) {
    size_t i;
    printf("%s:", label);
    for (i = 0; i < s.len; i++) {
        printf(" %d", s.ptr[i]);
    }
    printf("\n");
}

int main(void) {
    int data[] = {1, 2, 3, 4, 5, 6};
    int evens_buf[6];
    int squares_buf[6];
    i32_slice nums = i32_from(data, 6);
    i32_slice evens;
    i32_slice squares;
    int total;

    print_slice("numbers", nums);
    print_slice("first three", i32_take(nums, 3));

    evens = i32_filter(nums, evens_buf, is_even, NULL);
    squares = i32_map(evens, squares_buf, square, NULL);
    total = i32_fold(squares, 0, add, NULL);
    printf("evens squared sum: %d\n", total);

    return 0;
}
