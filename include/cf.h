/*
 * c-functional — functional programming for C99-C11.
 *
 * A header-only library of the classic functional combinators — map,
 * filter, fold, take, drop, find, any, all — over slices.
 *
 * Usage:
 *
 *     #include "cf.h"
 *
 *     CF_DEFINE(int, i32);   // typed wrappers for int, prefix i32
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
 *   - Typed, without macro abuse. The loops live exactly once, in
 *     real functions over untyped storage (the cf_core_* functions
 *     below — debuggable, compiled once, fixed in one place).
 *     CF_DEFINE only generates thin typed wrappers: one stub per
 *     combinator plus a 3-line adapter that casts elements back
 *     to T. A type mismatch is still a compile error, and user
 *     code never touches void* elements.
 *   - No closures in C, so every combinator takes a void *ctx
 *     context pointer (the qsort_r pattern). Pass NULL when the
 *     callback needs no state.
 *   - No hidden allocation. map/filter write into a caller-provided
 *     buffer with room for at least s.len elements; the library
 *     never calls malloc. The callback bundles are plain stack
 *     structs built by the wrappers. Function pointers are never
 *     cast through void*: only the bundle's address travels
 *     untyped, which is strictly conforming.
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
#include <string.h> /* memcpy */

#define CF_VERSION_MAJOR 0
#define CF_VERSION_MINOR 1
#define CF_VERSION_PATCH 0

#ifdef __cplusplus
extern "C" {
#endif

/*
 * CF_MAYBE_UNUSED — silence -Wunused-function for library functions
 * the user never calls. Clang warns about unused static functions
 * (gcc does not); a user who instantiates a type but only calls
 * three of its ten combinators must not get warnings. Unknown
 * compilers get an empty expansion.
 */
#if defined(__GNUC__)
#define CF_MAYBE_UNUSED __attribute__((unused))
#else
#define CF_MAYBE_UNUSED
#endif

/*
 * Generic core — one real implementation of each loop, over untyped
 * storage. The typed wrappers below forward to these; the adapters
 * translate between a typed callback and the untyped element
 * pointers the core walks. Element access is plain char* arithmetic.
 * The bundle is a small stack struct built by the wrapper, holding
 * the typed callback and the user's ctx.
 */
typedef void (*cf_map_adapter_fn)(void *out_elem, const void *in_elem, void *bundle);
typedef int (*cf_pred_adapter_fn)(const void *elem, void *bundle);
typedef void (*cf_fold_adapter_fn)(void *acc, const void *elem, void *bundle);

static inline CF_MAYBE_UNUSED void cf_core_map(const void *in, void *out,
        size_t len, size_t elem_size, cf_map_adapter_fn adapter, void *bundle) {
    const char *src = (const char *)in;
    char *dst = (char *)out;
    size_t i;
    for (i = 0; i < len; i++) {
        adapter(dst + i * elem_size, src + i * elem_size, bundle);
    }
}

static inline CF_MAYBE_UNUSED size_t cf_core_filter(const void *in, void *out,
        size_t len, size_t elem_size, cf_pred_adapter_fn pred, void *bundle) {
    const char *src = (const char *)in;
    char *dst = (char *)out;
    size_t i;
    size_t kept = 0;
    for (i = 0; i < len; i++) {
        const char *elem = src + i * elem_size;
        if (pred(elem, bundle)) {
            memcpy(dst + kept * elem_size, elem, elem_size);
            kept++;
        }
    }
    return kept;
}

static inline CF_MAYBE_UNUSED void cf_core_fold(void *acc, const void *in,
        size_t len, size_t elem_size, cf_fold_adapter_fn adapter, void *bundle) {
    const char *src = (const char *)in;
    size_t i;
    for (i = 0; i < len; i++) {
        adapter(acc, src + i * elem_size, bundle);
    }
}

static inline CF_MAYBE_UNUSED const void *cf_core_find(const void *in,
        size_t len, size_t elem_size, cf_pred_adapter_fn pred, void *bundle) {
    const char *src = (const char *)in;
    size_t i;
    for (i = 0; i < len; i++) {
        const void *elem = src + i * elem_size;
        if (pred(elem, bundle)) {
            return elem;
        }
    }
    return NULL;
}

static inline CF_MAYBE_UNUSED int cf_core_any(const void *in,
        size_t len, size_t elem_size, cf_pred_adapter_fn pred, void *bundle) {
    const char *src = (const char *)in;
    size_t i;
    for (i = 0; i < len; i++) {
        if (pred(src + i * elem_size, bundle)) {
            return 1;
        }
    }
    return 0;
}

static inline CF_MAYBE_UNUSED int cf_core_all(const void *in,
        size_t len, size_t elem_size, cf_pred_adapter_fn pred, void *bundle) {
    const char *src = (const char *)in;
    size_t i;
    for (i = 0; i < len; i++) {
        if (!pred(src + i * elem_size, bundle)) {
            return 0;
        }
    }
    return 1;
}

/*
 * CF_DEFINE(T, prefix) — thin typed wrappers over the generic core,
 * for element type T, with the given name prefix. The macro
 * generates declarations, one 3-line adapter per callback kind, and
 * one forwarding stub per combinator. No loop lives in the macro.
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
    typedef struct { prefix##_map_fn fn; void *ctx; } prefix##_map_bundle; \
    typedef struct { prefix##_pred fn; void *ctx; } prefix##_pred_bundle; \
    typedef struct { prefix##_fold_fn fn; void *ctx; } prefix##_fold_bundle; \
    static CF_MAYBE_UNUSED void prefix##_map_adapter(void *out_elem, \
            const void *in_elem, void *bundle) { \
        prefix##_map_bundle *b = (prefix##_map_bundle *)bundle; \
        *(T *)out_elem = b->fn(*(const T *)in_elem, b->ctx); \
    } \
    static CF_MAYBE_UNUSED int prefix##_pred_adapter(const void *elem, void *bundle) { \
        prefix##_pred_bundle *b = (prefix##_pred_bundle *)bundle; \
        return b->fn(*(const T *)elem, b->ctx); \
    } \
    static CF_MAYBE_UNUSED void prefix##_fold_adapter(void *acc, \
            const void *elem, void *bundle) { \
        prefix##_fold_bundle *b = (prefix##_fold_bundle *)bundle; \
        *(T *)acc = b->fn(*(T *)acc, *(const T *)elem, b->ctx); \
    } \
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
        prefix##_map_bundle b; \
        prefix##_slice result; \
        b.fn = fn; \
        b.ctx = ctx; \
        cf_core_map(s.ptr, out, s.len, sizeof(T), prefix##_map_adapter, &b); \
        result.ptr = out; \
        result.len = s.len; \
        return result; \
    } \
    static inline CF_MAYBE_UNUSED prefix##_slice prefix##_filter(prefix##_slice s, T *out, \
            prefix##_pred pred, void *ctx) { \
        prefix##_pred_bundle b; \
        prefix##_slice result; \
        b.fn = pred; \
        b.ctx = ctx; \
        result.ptr = out; \
        result.len = cf_core_filter(s.ptr, out, s.len, sizeof(T), \
            prefix##_pred_adapter, &b); \
        return result; \
    } \
    static inline CF_MAYBE_UNUSED T prefix##_fold(prefix##_slice s, T init, \
            prefix##_fold_fn fn, void *ctx) { \
        prefix##_fold_bundle b; \
        T acc = init; \
        b.fn = fn; \
        b.ctx = ctx; \
        cf_core_fold(&acc, s.ptr, s.len, sizeof(T), prefix##_fold_adapter, &b); \
        return acc; \
    } \
    static inline CF_MAYBE_UNUSED T *prefix##_find(prefix##_slice s, prefix##_pred pred, \
            void *ctx) { \
        prefix##_pred_bundle b; \
        b.fn = pred; \
        b.ctx = ctx; \
        return (T *)cf_core_find(s.ptr, s.len, sizeof(T), prefix##_pred_adapter, &b); \
    } \
    static inline CF_MAYBE_UNUSED int prefix##_any(prefix##_slice s, prefix##_pred pred, \
            void *ctx) { \
        prefix##_pred_bundle b; \
        b.fn = pred; \
        b.ctx = ctx; \
        return cf_core_any(s.ptr, s.len, sizeof(T), prefix##_pred_adapter, &b); \
    } \
    static inline CF_MAYBE_UNUSED int prefix##_all(prefix##_slice s, prefix##_pred pred, \
            void *ctx) { \
        prefix##_pred_bundle b; \
        b.fn = pred; \
        b.ctx = ctx; \
        return cf_core_all(s.ptr, s.len, sizeof(T), prefix##_pred_adapter, &b); \
    } \
    typedef char prefix##_cf_defined[1]

#ifdef __cplusplus
}
#endif

#endif /* CF_H */
