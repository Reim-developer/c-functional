/*
 * Tests for the c-functional core: map, filter, fold, take, drop,
 * find, any, all. Run with: make test   (full matrix: make check)
 *
 * Deliberately includes cf.h twice to prove the include guard works.
 */

#include <stdio.h>

#include "cf.h"
#include "cf.h"

struct point {
    int x;
    int y;
};

CF_DEFINE(int, i32);
CF_DEFINE(double, f64);
CF_DEFINE(struct point, point);

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond) do { \
    if (cond) { \
        g_passed++; \
    } else { \
        g_failed++; \
        fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond); \
    } \
} while (0)

/* --- int callbacks ------------------------------------------------ */

static int is_even(int v, void *ctx) {
    (void)ctx;
    return v % 2 == 0;
}

static int is_positive(int v, void *ctx) {
    (void)ctx;
    return v > 0;
}

static int greater_than(int v, void *ctx) {
    return v > *(const int *)ctx;
}

static int double_it(int v, void *ctx) {
    (void)ctx;
    return v * 2;
}

static int add_offset(int v, void *ctx) {
    return v + *(const int *)ctx;
}

static int add(int acc, int v, void *ctx) {
    (void)ctx;
    return acc + v;
}

static int multiply(int acc, int v, void *ctx) {
    (void)ctx;
    return acc * v;
}

/* counts predicate calls through ctx, proving short-circuiting */
struct counter {
    int calls;
};

static int count_and_even(int v, void *ctx) {
    struct counter *c = ctx;
    c->calls++;
    return v % 2 == 0;
}

static int count_and_positive(int v, void *ctx) {
    struct counter *c = ctx;
    c->calls++;
    return v > 0;
}

/* --- double callbacks ---------------------------------------------- */

static int f64_is_positive(double v, void *ctx) {
    (void)ctx;
    return v > 0.0;
}

static double f64_scale(double v, void *ctx) {
    return v * *(const double *)ctx;
}

static double f64_add(double acc, double v, void *ctx) {
    (void)ctx;
    return acc + v;
}

/* --- point callbacks ----------------------------------------------- */

static int point_x_gt(struct point p, void *ctx) {
    return p.x > *(const int *)ctx;
}

static struct point point_sum(struct point acc, struct point p, void *ctx) {
    struct point out;
    (void)ctx;
    out.x = acc.x + p.x;
    out.y = acc.y + p.y;
    return out;
}

static struct point point_shift(struct point p, void *ctx) {
    struct point out;
    const struct point *d = ctx;
    out.x = p.x + d->x;
    out.y = p.y + d->y;
    return out;
}

/* --- tests ---------------------------------------------------------- */

static void test_take_drop(void) {
    int data[] = {10, 20, 30, 40, 50};
    i32_slice s = i32_from(data, 5);
    i32_slice empty = i32_from(NULL, 0);
    i32_slice t;
    i32_slice d;

    /* take */
    t = i32_take(s, 0);
    CHECK(t.ptr == data && t.len == 0);
    t = i32_take(s, 3);
    CHECK(t.ptr == data && t.len == 3);
    CHECK(t.ptr[0] == 10 && t.ptr[2] == 30);
    t = i32_take(s, 5);
    CHECK(t.ptr == data && t.len == 5);
    t = i32_take(s, 99); /* clamped */
    CHECK(t.ptr == data && t.len == 5);
    t = i32_take(empty, 3);
    CHECK(t.ptr == NULL && t.len == 0);

    /* drop */
    d = i32_drop(s, 0);
    CHECK(d.ptr == data && d.len == 5);
    d = i32_drop(s, 2);
    CHECK(d.ptr == data + 2 && d.len == 3);
    CHECK(d.ptr[0] == 30 && d.ptr[2] == 50);
    d = i32_drop(s, 5);
    CHECK(d.ptr == data + 5 && d.len == 0);
    d = i32_drop(s, 99); /* clamped */
    CHECK(d.ptr == data + 5 && d.len == 0);
    d = i32_drop(empty, 3);
    CHECK(d.ptr == NULL && d.len == 0);

    /* take/drop are O(1) views: no copy, same storage */
    t = i32_take(s, 2);
    t.ptr[0] = 11;
    CHECK(data[0] == 11);
    data[0] = 10;

    /* empty helper */
    CHECK(i32_empty(empty));
    CHECK(!i32_empty(s));
    CHECK(i32_empty(i32_take(s, 0)));
    CHECK(i32_empty(i32_drop(s, 5)));
}

static void test_map(void) {
    int data[] = {1, 2, 3, 4};
    int out[4];
    int offset = 10;
    i32_slice s = i32_from(data, 4);
    i32_slice empty = i32_from(NULL, 0);
    i32_slice r;

    r = i32_map(s, out, double_it, NULL);
    CHECK(r.ptr == out && r.len == 4);
    CHECK(out[0] == 2 && out[1] == 4 && out[2] == 6 && out[3] == 8);
    /* source untouched */
    CHECK(data[0] == 1 && data[3] == 4);

    r = i32_map(s, out, add_offset, &offset);
    CHECK(out[0] == 11 && out[3] == 14);

    /* empty input: out may be NULL, nothing is written */
    r = i32_map(empty, NULL, double_it, NULL);
    CHECK(r.ptr == NULL && r.len == 0);
}

static void test_filter(void) {
    int data[] = {1, 2, 3, 4, 5, 6};
    int all_pos[] = {1, 2, 3};
    int all_neg[] = {-1, -2, -3};
    int out[6];
    int threshold = 3;
    i32_slice s = i32_from(data, 6);
    i32_slice empty = i32_from(NULL, 0);
    i32_slice r;

    r = i32_filter(s, out, is_even, NULL);
    CHECK(r.ptr == out && r.len == 3);
    CHECK(out[0] == 2 && out[1] == 4 && out[2] == 6);

    /* ctx-carrying predicate */
    r = i32_filter(s, out, greater_than, &threshold);
    CHECK(r.len == 3);
    CHECK(out[0] == 4 && out[1] == 5 && out[2] == 6);

    /* nothing matches */
    r = i32_filter(i32_from(all_neg, 3), out, is_positive, NULL);
    CHECK(r.len == 0);

    /* everything matches */
    r = i32_filter(i32_from(all_pos, 3), out, is_positive, NULL);
    CHECK(r.len == 3);
    CHECK(out[0] == 1 && out[2] == 3);

    /* empty input */
    r = i32_filter(empty, out, is_even, NULL);
    CHECK(r.len == 0);
}

static void test_fold(void) {
    int data[] = {1, 2, 3, 4};
    int mult = 3;
    i32_slice s = i32_from(data, 4);
    i32_slice empty = i32_from(NULL, 0);

    CHECK(i32_fold(s, 0, add, NULL) == 10);
    CHECK(i32_fold(s, 100, add, NULL) == 110);
    CHECK(i32_fold(s, 1, multiply, NULL) == 24);
    /* empty: init comes back untouched */
    CHECK(i32_fold(empty, 42, add, NULL) == 42);
    /* ctx is passed through */
    CHECK(i32_fold(s, 0, add, &mult) == 10);
}

static void test_find(void) {
    int data[] = {1, 3, 5, 8, 9};
    int threshold = 4;
    int *p;
    i32_slice s = i32_from(data, 5);
    i32_slice empty = i32_from(NULL, 0);

    p = i32_find(s, is_even, NULL);
    CHECK(p != NULL && *p == 8 && p == data + 3);

    p = i32_find(s, greater_than, &threshold);
    CHECK(p != NULL && *p == 5 && p == data + 2);

    /* missing */
    p = i32_find(s, greater_than, &(int){100});
    CHECK(p == NULL);

    /* empty */
    p = i32_find(empty, is_even, NULL);
    CHECK(p == NULL);
}

static void test_any_all(void) {
    int mixed[] = {1, 2, 3};
    int evens[] = {2, 4, 6};
    int odds[] = {1, 3, 5};
    struct counter c;
    i32_slice empty = i32_from(NULL, 0);

    CHECK(i32_any(i32_from(mixed, 3), is_even, NULL));
    CHECK(!i32_any(i32_from(odds, 3), is_even, NULL));
    CHECK(!i32_any(empty, is_even, NULL)); /* empty: false */

    CHECK(i32_all(i32_from(evens, 3), is_even, NULL));
    CHECK(!i32_all(i32_from(mixed, 3), is_even, NULL));
    CHECK(i32_all(empty, is_even, NULL)); /* empty: vacuous truth */

    /* any short-circuits at the first match (2 calls, not 3) */
    c.calls = 0;
    CHECK(i32_any(i32_from(mixed, 3), count_and_even, &c));
    CHECK(c.calls == 2);

    /* all short-circuits at the first failure (1 call, not 3) */
    c.calls = 0;
    CHECK(i32_all(i32_from(mixed, 3), count_and_positive, &c));
    /* mixed is all positive, so all 3 are checked */
    CHECK(c.calls == 3);
    c.calls = 0;
    {
        int with_neg[] = {-1, 2, 3};
        CHECK(!i32_all(i32_from(with_neg, 3), count_and_positive, &c));
        CHECK(c.calls == 1);
    }
}

static void test_double(void) {
    double data[] = {1.5, -2.0, 3.5};
    double out[3];
    double factor = 2.0;
    double *p;
    f64_slice s = f64_from(data, 3);
    f64_slice empty = f64_from(NULL, 0);
    f64_slice r;

    r = f64_map(s, out, f64_scale, &factor);
    CHECK(r.len == 3);
    CHECK(out[0] == 3.0 && out[1] == -4.0 && out[2] == 7.0);

    r = f64_filter(s, out, f64_is_positive, NULL);
    CHECK(r.len == 2);
    CHECK(out[0] == 1.5 && out[1] == 3.5);

    CHECK(f64_fold(s, 0.0, f64_add, NULL) == 3.0);
    CHECK(f64_fold(empty, 9.5, f64_add, NULL) == 9.5);
    CHECK(f64_any(s, f64_is_positive, NULL));
    CHECK(!f64_all(s, f64_is_positive, NULL));
    CHECK(!f64_any(empty, f64_is_positive, NULL));
    CHECK(f64_all(empty, f64_is_positive, NULL));

    r = f64_take(s, 2);
    CHECK(r.ptr == data && r.len == 2 && r.ptr[1] == -2.0);
    r = f64_take(s, 99);
    CHECK(r.len == 3);
    r = f64_drop(s, 1);
    CHECK(r.ptr == data + 1 && r.len == 2 && r.ptr[0] == -2.0);
    r = f64_drop(s, 99);
    CHECK(r.len == 0);

    p = f64_find(s, f64_is_positive, NULL);
    CHECK(p != NULL && *p == 1.5 && p == data);
    p = f64_find(empty, f64_is_positive, NULL);
    CHECK(p == NULL);

    CHECK(f64_empty(empty));
    CHECK(!f64_empty(s));
}

static void test_point(void) {
    struct point pts[] = {{1, 1}, {5, 2}, {3, 3}, {9, 4}};
    struct point out[4];
    struct point zero;
    struct point shift;
    struct point total;
    int threshold = 4;
    struct point *p;
    point_slice s = point_from(pts, 4);
    point_slice empty = point_from(NULL, 0);
    point_slice r;

    zero.x = 0;
    zero.y = 0;
    shift.x = 10;
    shift.y = 100;

    r = point_filter(s, out, point_x_gt, &threshold);
    CHECK(r.len == 2);
    CHECK(out[0].x == 5 && out[1].x == 9);

    p = point_find(s, point_x_gt, &threshold);
    CHECK(p != NULL && p->x == 5 && p->y == 2);
    p = point_find(empty, point_x_gt, &threshold);
    CHECK(p == NULL);

    total = point_fold(s, zero, point_sum, NULL);
    CHECK(total.x == 18 && total.y == 10);
    total = point_fold(empty, zero, point_sum, NULL);
    CHECK(total.x == 0 && total.y == 0);

    /* map over a struct type, with ctx */
    r = point_map(s, out, point_shift, &shift);
    CHECK(r.len == 4);
    CHECK(out[0].x == 11 && out[0].y == 101);
    CHECK(out[3].x == 19 && out[3].y == 104);

    r = point_take(s, 2);
    CHECK(r.ptr == pts && r.len == 2 && r.ptr[1].x == 5);
    r = point_drop(s, 3);
    CHECK(r.ptr == pts + 3 && r.len == 1 && r.ptr[0].x == 9);

    CHECK(point_any(s, point_x_gt, &threshold));
    CHECK(!point_all(s, point_x_gt, &threshold));
    CHECK(!point_any(empty, point_x_gt, &threshold));
    CHECK(point_all(empty, point_x_gt, &threshold));

    CHECK(point_empty(empty));
    CHECK(!point_empty(s));
}

static void test_composition(void) {
    /* take -> filter -> fold, the classic pipeline */
    int data[] = {1, 2, 3, 4, 5, 6, 7, 8};
    int buf[8];
    int total;
    i32_slice s = i32_from(data, 8);

    /* first 6, keep evens (2,4,6), sum = 12 */
    total = i32_fold(
        i32_filter(i32_take(s, 6), buf, is_even, NULL), 0, add, NULL);
    CHECK(total == 12);

    /* drop 5, keep evens (6,8), sum = 14 */
    total = i32_fold(
        i32_filter(i32_drop(s, 5), buf, is_even, NULL), 0, add, NULL);
    CHECK(total == 14);

    /* map then fold: double everything, sum = 2 * 36 = 72 */
    total = i32_fold(i32_map(s, buf, double_it, NULL), 0, add, NULL);
    CHECK(total == 72);
}

int main(void) {
    test_take_drop();
    test_map();
    test_filter();
    test_fold();
    test_find();
    test_any_all();
    test_double();
    test_point();
    test_composition();

    printf("test_cf: %d passed, %d failed\n", g_passed, g_failed);
    return g_failed != 0 ? 1 : 0;
}
