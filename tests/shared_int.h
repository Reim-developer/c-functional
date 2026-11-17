/*
 * Shared header for the multi-translation-unit test: CF_DEFINE lives
 * in a header included by two .c files, proving the generated static
 * inline functions link cleanly across translation units.
 */

#ifndef SHARED_INT_H
#define SHARED_INT_H

#include <stddef.h>

#include "cf.h"

CF_DEFINE(int, shared_i32);

int shared_sum(const int *data, size_t n);

#endif /* SHARED_INT_H */
