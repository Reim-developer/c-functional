/*
 * Translation unit A of the multi-TU test: implements shared_sum
 * using the combinators from shared_int.h.
 */

#include "shared_int.h"

static int shared_add(int acc, int v, void *ctx) {
    (void)ctx;
    return acc + v;
}

int shared_sum(const int *data, size_t n) {
    /* the slice borrows the caller's array; fold only reads it */
    return shared_i32_fold(shared_i32_from((int *)data, n), 0,
        shared_add, NULL);
}
