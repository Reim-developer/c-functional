/*
 * c-functional — functional programming for C99-C11.
 *
 * A header-only library of the classic functional combinators — map,
 * filter, fold, take, drop, find, any, all — over slices, with full
 * type safety from macro-generated typed functions.
 *
 * Usage:
 *
 *     #include "cf.h"
 *
 *     CF_DEFINE(int, i32);   // generates i32_slice, i32_map, ...
 *
 *     int data[] = {1, 2, 3, 4, 5, 6};
 *     int buf[6];
 *     i32_slice nums   = i32_from(data, 6);
 *     i32_slice evens  = i32_filter(nums, buf, is_even, NULL);
 *     int       total  = i32_fold(evens, 0, add, NULL);
 *
 * Design:
 *
 *   - Slices. A slice is a {ptr, len} view over an array you own.
 *     take/drop are O(1) views: they never copy.
 *   - Typed. CF_DEFINE generates real functions for your element
 *     type, so a type mismatch is a compile error, not a runtime
 *     surprise. There is no void* element handling anywhere.
 *   - No closures in C, so every combinator takes a void *ctx
 *     context pointer (the qsort_r pattern). Pass NULL when the
 *     callback needs no state.
 *   - No hidden allocation. map/filter write into a caller-provided
 *     buffer with room for at least s.len elements; the library
 *     never calls malloc.
 *   - C99 baseline. No _Generic, no typeof, no _Static_assert, no
 *     designated initializers, so the header works from C99 through
 *     C23 and on old compilers. Invoke CF_DEFINE once per (type,
 *     prefix) per translation unit, at file scope.
 *
 * Empty slices are always safe: every combinator handles len == 0
 * (and a NULL ptr with len == 0) without dereferencing anything.
 * fold over an empty slice returns init; any over an empty slice is
 * false; all over an empty slice is true (vacuous truth).
 */

#ifndef CF_H
#define CF_H

#include <stddef.h> /* size_t, NULL */

#define CF_VERSION_MAJOR 0
#define CF_VERSION_MINOR 1
#define CF_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CF_MAYBE_UNUSED — silence -Wunused-function for generated
 * combinators the user never calls. Clang warns about unused static
 * inline functions (gcc does not); a user who instantiates a type
 * but only calls three of its ten combinators must not get warnings.
 * Unknown compilers get an empty expansion.
 */
#if defined(__GNUC__)
#define CF_MAYBE_UNUSED __attribute__((unused))
#else
#define CF_MAYBE_UNUSED
#endif

/*
 * CF_DEFINE(T, prefix) — generate the slice type and all combinators
 * for element type T, with the given name prefix.
 *
 * Generated:
 *
 *   prefix_slice         struct { T *ptr; size_t len; }
 *   prefix_pred          int (*)(T value, void *ctx)
 *   prefix_map_fn        T   (*)(T value, void *ctx)
 *   prefix_fold_fn       T   (*)(T acc, T value, void *ctx)
 *
 *   prefix_from(ptr, len)          build a slice from a pointer + length
 *   prefix_empty(s)                1 if s.len == 0, else 0
 *   prefix_take(s, n)              first n elements (clamped to s.len)
 *   prefix_drop(s, n)              s without its first n elements
 *                                  (clamped; n >= s.len gives empty)
 *   prefix_map(s, out, fn, ctx)    fn over every element into out,
 *                                  returns a slice over out
 *   prefix_filter(s, out, p, ctx)  elements passing p, into out,
 *                                  returns a slice over out
 *   prefix_fold(s, init, fn, ctx)  reduce left to right from init
 *   prefix_find(s, pred, ctx)      pointer to the first element passing
 *                                  pred, or NULL
 *   prefix_any(s, pred, ctx)       1 if any element passes pred
 *   prefix_all(s, pred, ctx)       1 if every element passes pred
 *
 * take/drop return views into the same storage (O(1), no copy).
 * map/filter require out to hold at least s.len elements; they return
 * a slice over out, so results compose: fold(filter(take(s, n), ...)).
 * find/any/all short-circuit at the first decisive element.
 *
 * T must be a plain type name; typedef anything complicated first.
 * Invoke at file scope, once per prefix per translation unit.
 */
#define CF_DEFINE(T, prefix) \
    typedef struct { T *ptr; size_t len; } prefix##_slice; \
    typedef int (*prefix##_pred)(T value, void *ctx); \
    typedef T (*prefix##_map_fn)(T value, void *ctx); \
    typedef T (*prefix##_fold_fn)(T acc, T value, void *ctx); \
    static inline CF_MAYBE_UNUSED prefix##_slice prefix##_from(T *ptr, size_t len) { \
        prefix##_slice s; \
        s.ptr = ptr; \
        s.len = len; \
        return s; \
    } \
    static inline CF_MAYBE_UNUSED int prefix##_empty(prefix##_slice s) { \
        return s.len == 0; \
    } \
    static inline CF_MAYBE_UNUSED prefix##_slice prefix##_take(prefix##_slice s, size_t n) { \
        prefix##_slice out; \
        out.ptr = s.ptr; \
        out.len = n < s.len ? n : s.len; \
        return out; \
    } \
    static inline CF_MAYBE_UNUSED prefix##_slice prefix##_drop(prefix##_slice s, size_t n) { \
        prefix##_slice out; \
        if (n >= s.len) { \
            out.ptr = s.len != 0 ? s.ptr + s.len : s.ptr; \
            out.len = 0; \
        } else { \
            out.ptr = s.ptr + n; \
            out.len = s.len - n; \
        } \
        return out; \
    } \
    static inline CF_MAYBE_UNUSED prefix##_slice prefix##_map(prefix##_slice s, T *out, \
            prefix##_map_fn fn, void *ctx) { \
        prefix##_slice result; \
        size_t i; \
        result.ptr = out; \
        result.len = s.len; \
        for (i = 0; i < s.len; i++) { \
            out[i] = fn(s.ptr[i], ctx); \
        } \
        return result; \
    } \
    static inline CF_MAYBE_UNUSED prefix##_slice prefix##_filter(prefix##_slice s, T *out, \
            prefix##_pred pred, void *ctx) { \
        prefix##_slice result; \
        size_t i; \
        size_t kept = 0; \
        for (i = 0; i < s.len; i++) { \
            if (pred(s.ptr[i], ctx)) { \
                out[kept] = s.ptr[i]; \
                kept++; \
            } \
        } \
        result.ptr = out; \
        result.len = kept; \
        return result; \
    } \
    static inline CF_MAYBE_UNUSED T prefix##_fold(prefix##_slice s, T init, \
            prefix##_fold_fn fn, void *ctx) { \
        T acc = init; \
        size_t i; \
        for (i = 0; i < s.len; i++) { \
            acc = fn(acc, s.ptr[i], ctx); \
        } \
        return acc; \
    } \
    static inline CF_MAYBE_UNUSED T *prefix##_find(prefix##_slice s, prefix##_pred pred, \
            void *ctx) { \
        size_t i; \
        for (i = 0; i < s.len; i++) { \
            if (pred(s.ptr[i], ctx)) { \
                return s.ptr + i; \
            } \
        } \
        return NULL; \
    } \
    static inline CF_MAYBE_UNUSED int prefix##_any(prefix##_slice s, prefix##_pred pred, \
            void *ctx) { \
        size_t i; \
        for (i = 0; i < s.len; i++) { \
            if (pred(s.ptr[i], ctx)) { \
                return 1; \
            } \
        } \
        return 0; \
    } \
    static inline CF_MAYBE_UNUSED int prefix##_all(prefix##_slice s, prefix##_pred pred, \
            void *ctx) { \
        size_t i; \
        for (i = 0; i < s.len; i++) { \
            if (!pred(s.ptr[i], ctx)) { \
                return 0; \
            } \
        } \
        return 1; \
    } \
    typedef char prefix##_cf_defined[1]

#ifdef __cplusplus
}
#endif

#endif /* CF_H */
