/*
 * Regression test for unused-combinator warnings: this TU instantiates
 * a type and calls NONE of its generated functions. It must compile
 * clean under -Wall -Wextra -Werror -pedantic on both gcc and clang —
 * clang warns about unused static inline functions, so the header
 * marks every generated function with __attribute__((unused)).
 */

#include "cf.h"

CF_DEFINE(long, unused_l);

int main(void) {
    return 0;
}
