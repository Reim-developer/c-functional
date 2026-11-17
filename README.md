# c-functional

Functional programming for C99-C11: map, filter, fold, take. Header-only.

The classic functional combinators over slices, with real type safety:
`CF_DEFINE` generates typed functions for your element type, so a type
mismatch is a compile error — not a `void*` cast that fails at runtime.

## Features

- `map`, `filter`, `fold`, `take`, `drop`, `find`, `any`, `all` over slices
- **Typed.** One macro generates the whole API for your element type.
- **Header-only.** Copy `include/cf.h` into your project; nothing to link.
- **C99 baseline.** No `_Generic`, no `typeof`, no designated initializers —
  it compiles from C99 through C23, including on old compilers.
- **No hidden allocation.** `map`/`filter` write into a buffer you provide;
  the library never calls `malloc`.
- **No closures in C**, so every combinator takes a `void *ctx` context
  pointer (the `qsort_r` pattern). Pass `NULL` when no state is needed.
- **`take`/`drop` are O(1) views** — they never copy.

## Quick start

```c
#include "cf.h"

CF_DEFINE(int, i32);   /* generates i32_slice, i32_map, i32_filter, ... */

int data[] = {1, 2, 3, 4, 5, 6};
int buf[6];
i32_slice nums  = i32_from(data, 6);
i32_slice evens = i32_filter(nums, buf, is_even, NULL);
int total       = i32_fold(evens, 0, add, NULL);
```

## Usage

A full program (`examples/quickstart.c` — compiled and run by `make test`):

```c
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
```

Output:

```
numbers: 1 2 3 4 5 6
first three: 1 2 3
evens squared sum: 56
```

## API

`CF_DEFINE(T, prefix)` generates the following, all `static inline`:

| Generated | Description |
| --- | --- |
| `prefix_slice` | `struct { T *ptr; size_t len; }` — a view over your array |
| `prefix_pred` | `int (*)(T value, void *ctx)` |
| `prefix_map_fn` | `T (*)(T value, void *ctx)` |
| `prefix_fold_fn` | `T (*)(T acc, T value, void *ctx)` |
| `prefix_from(ptr, len)` | Build a slice from a pointer and length |
| `prefix_empty(s)` | `1` if `s.len == 0` |
| `prefix_take(s, n)` | First `n` elements, clamped to `s.len` |
| `prefix_drop(s, n)` | `s` without its first `n` elements; `n >= s.len` gives empty |
| `prefix_map(s, out, fn, ctx)` | `fn` over every element into `out`; returns a slice over `out` |
| `prefix_filter(s, out, p, ctx)` | Elements passing `p`, into `out`; returns a slice over `out` |
| `prefix_fold(s, init, fn, ctx)` | Reduce left to right, starting from `init` |
| `prefix_find(s, pred, ctx)` | Pointer to the first element passing `pred`, or `NULL` |
| `prefix_any(s, pred, ctx)` | `1` if any element passes `pred` (short-circuits) |
| `prefix_all(s, pred, ctx)` | `1` if every element passes `pred` (short-circuits) |

Rules:

- Invoke `CF_DEFINE` at file scope, once per `(type, prefix)` per
  translation unit. `T` must be a plain type name — `typedef` anything
  complicated first.
- `map`/`filter` require `out` to hold at least `s.len` elements. Their
  return value is a slice over `out`, so results compose:
  `fold(filter(take(s, n), ...))`.
- Empty slices are always safe: every combinator handles `len == 0`
  (and a `NULL` pointer with `len == 0`) without dereferencing anything.
  `fold` over an empty slice returns `init`; `any` is false; `all` is
  true (vacuous truth).

## Tests

```sh
make check   # gcc and clang, each at -std=c99 and -std=c11
make test    # same suite with $(CC) at -std=c99
```

Everything is compiled with `-Wall -Wextra -Werror -pedantic`.

## License

MIT — see [LICENSE](LICENSE).
