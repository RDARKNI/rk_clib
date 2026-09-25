// SPDX-License-Identifier: MIT
/// @file rk_heap.h
/// @version 1.0.0
/// @defgroup rk_heap Heap (Binary Min-Heap) Interface
/// @brief Type-safe binary min-heap backed by rk_vec.h.
///
/// Define a specialization once at file scope with `HEAP_DEFINE(T, CMP_FUN)`. `CMP_FUN` has the
/// same convention as the tree comparators: `int cmp(T a, T b)` returns negative, zero, or
/// positive. The smallest value is at the root. To make a max-heap, supply a comparator with the
/// opposite ordering.
///
/// Usage:
/// ```c
/// static int int_cmp(int a, int b) { return (a > b) - (a < b); }
/// HEAP_DEFINE(int, int_cmp);
/// Heap(int) h = heap_init(int, 0);
/// heap_push(int, &h, 4);
/// heap_push(int, &h, 2);
/// int smallest = heap_pop(int, &h); // 2
/// heap_release(&h);
/// ```
///
/// Push and pop take O(log n); peek takes O(1). Equal values are allowed. The order among equal
/// values is unspecified. As with `Vec`, pointers into the heap's data may be invalidated by
/// insertion or removal.
///
/// If all values are known up front, building the Heap in O(n) overall beats n separate
/// O(log n) `heap_push()` calls. Four entry points cover this, differing in whether they allocate
/// fresh storage or reuse existing storage, and whether prior contents are kept or discarded:
/// - `heap_from(T, arr, n, alloc?)` - fresh Heap, copying `arr`.
/// - `heap_adopt(T, vec)` - fresh Heap, taking ownership of an existing `Vec(T)` with no copy.
/// - `heap_assign(T, self, arr, n)` - replaces an existing Heap's contents with `arr`, reusing its
///   backing Vec.
/// - `heap_extend(T, self, arr, n)` - appends `arr` to an existing Heap's current contents, reusing
///   its backing Vec.
///
/// `heap_replace_top()` combines a pop and a push into a single sift-down; prefer it over a
/// separate `heap_pop()`/`heap_push()` pair when repeatedly replacing the minimum (e.g. k-way
/// merges, running top-k selection).
/// @see rk_vec.h
/// @{

#ifndef RK_HEAP_H
#define RK_HEAP_H
#include "rk_vec.h"
RK_HEADER_BEGIN

/// @brief Define a heap type and functions for a given element type.
///
/// This macro generates a complete, type-specific binary min-heap API for the given element type.
///
/// @param T       Name of the element type. Must be an identifier; use a typedef for a pointer or
///                struct type.
/// @param CMP_FUN Comparison function (`int CMP_FUN(T a, T b)`), returning negative, zero, or
///                positive, following the same convention as `strcmp` and the tree comparators.
/// @attention Must be invoked at file scope, once per `T`.
#define HEAP_DEFINE(T, CMP_FUN)          RK__HEAP_DEFINE(T, CMP_FUN)

/// @brief Macro to indicate that an object is a Heap.
/// @param T The type of elements stored in the Heap
#define Heap(T)                          rk_heap_##T

/// @brief `Heap(T) heap_init(T, size_t cap, Allocator alloc = alloc_ctx)` - Initialises and
/// returns an empty Heap.
/// @param T        The desired type of the Heap's elements
/// @param cap      The initial capacity of the backing Vec (in elements)
/// @param alloc    Optional allocator; defaults to `alloc_ctx`. See `vec_init()`.
/// @return An initialised, empty `Heap(T)`
/// @note A zero-initialized `Heap(T)` is also a valid, empty heap.
#define heap_init(T, cap, ...)           ((Heap(T)){.data = vec_init(T, cap, ##__VA_ARGS__)})

/// @brief `Heap(T) heap_from(T, const T* arr, size_t n, Allocator alloc = alloc_ctx)` - Constructs
/// a new Heap by copying `n` values from `arr` and heapifying them, in O(n) overall.
/// @param T     Element type
/// @param arr   Source array of `n` values
/// @param n     Number of values to copy
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return A new `Heap(T)` containing a heap-ordered copy of `arr`'s first `n` values
#define heap_from(T, arr, n, ...)        rk_overload(RK__HEAP_FROM, T, arr, n, ##__VA_ARGS__)

/// @brief `Heap(T) heap_adopt(T, Vec(T) vec)` - Constructs a new Heap by taking ownership of `vec`
/// and heapifying it in place, with no copy.
/// @param T   Element type
/// @param vec An existing `Vec(T)`, passed by value
/// @return A `Heap(T)` wrapping `vec`'s own storage, now in heap order
/// @attention `vec` is consumed: its storage now belongs to the returned Heap. Do not read, mutate,
/// or `vec_release()` the original `vec` variable afterwards; release the Heap instead.
#define heap_adopt(T, vec)               RK__HEAP_F(T, adopt)(vec)

/// @brief `void heap_release(Heap(T)* self)` - Frees the backing Vec and resets the Heap to an
/// empty state.
#define heap_release(self)               vec_release((self)->data)

/// @brief `size_t heap_count(Heap(T)* self)` - Returns the number of elements stored in the Heap.
#define heap_count(self)                 vec_count((self)->data)

/// @brief `size_t heap_cap(Heap(T)* self)` - Returns the current capacity of the backing Vec.
#define heap_cap(self)                   vec_cap((self)->data)

/// @brief `Allocator heap_allocator(Heap(T)* self)` - Returns the Allocator the Heap's backing Vec
/// was constructed with.
#define heap_allocator(self)             vec_allocator((self)->data)

/// @brief `bool heap_is_empty(Heap(T)* self)` - Returns `true` iff the Heap contains no elements.
#define heap_is_empty(self)              (heap_count(self) == 0)

/// @brief `void heap_clear(Heap(T)* self)` - Removes all elements without freeing the backing Vec.
#define heap_clear(self)                 vec_clear((self)->data)

/// @brief `void heap_reserve(Heap(T)* self, size_t cap)` - Ensures the backing Vec can hold at
/// least `cap` elements without reallocating.
/// @param cap Minimum capacity to reserve (in elements)
/// @attention **Arguments with side effects are not safe in `vec_`-backed macros**
#define heap_reserve(self, cap)          vec_reserve((self)->data, cap)

/// @brief `void heap_shrink_to_fit(Heap(T)* self)` - Shrinks the backing Vec's capacity to the next
/// power of two greater than or equal to its length (matching `vec_shrink_to_fit()`'s convention).
/// @attention **Arguments with side effects are not safe in `vec_`-backed macros**
/// @note Safe to call at any time: shrinking never touches element order, so the heap invariant is
/// unaffected. Frees the backing Vec entirely if the Heap is empty.
#define heap_shrink_to_fit(self)         vec_shrink_to_fit((self)->data)

/// @brief `void heap_assign(T, Heap(T)* self, const T* arr, size_t n)` - Replaces the Heap's
/// contents with a heap-ordered copy of `arr`'s first `n` values, reusing the existing backing
/// Vec's buffer (growing it if necessary) rather than allocating a new one.
/// @param T   Element type
/// @param arr Source array of `n` values
/// @param n   Number of values to copy
/// @attention `arr[0..n)` must not overlap the Heap's own backing allocation: if growth is
/// triggered, the old buffer is freed before the copy from `arr` happens, turning an `arr` that
/// points into it into a use-after-free; even without growth, the underlying copy is a plain
/// `memcpy`, which is undefined for overlapping source and destination.
#define heap_assign(T, self, arr, n)     RK__HEAP_F(T, assign)((self), (arr), (n))

/// @brief `const T* heap_peek(T, const Heap(T)* self)` - Returns a pointer to the minimum element
/// without removing it.
/// @param T Element type
/// @return Pointer to the minimum element, or `NULL` if the Heap is empty
/// @note Invalidated by any later mutation of the Heap.
#define heap_peek(T, self)               RK__HEAP_F(T, peek)(self)

/// @brief `void heap_push(T, Heap(T)* self, T value)` - Inserts `value` into the Heap.
/// @param T     Element type
/// @param value Value to insert. Evaluated once.
/// @note A push may reallocate the backing Vec, invalidating prior pointers into it.
#define heap_push(T, self, value)        RK__HEAP_F(T, push)(self, value)

/// @brief `T heap_pop(T, Heap(T)* self)` - Removes and returns the minimum element.
/// @param T Element type
/// @return The (former) minimum element
/// @attention Requires a nonempty Heap.
#define heap_pop(T, self)                RK__HEAP_F(T, pop)(self)

/// @brief `bool heap_try_pop(T, Heap(T)* self, T* out)` - Removes the minimum element and writes
/// it to `*out`, if the Heap is nonempty.
/// @param T   Element type
/// @param out Destination for the removed value. Left untouched if the Heap is empty.
/// @return `true` if an element was removed, `false` if the Heap was empty
#define heap_try_pop(T, self, out)       RK__HEAP_F(T, try_pop)(self, out)

/// @brief `T heap_replace_top(T, Heap(T)* self, T value)` - Removes the minimum element and
/// inserts `value`, in a single sift-down.
/// @param T     Element type
/// @param value Value to insert in place of the removed minimum. Evaluated once.
/// @return The (former) minimum element
/// @attention Requires a nonempty Heap.
/// @note Equivalent to, but cheaper than, `heap_pop()` followed by `heap_push()`: it never shrinks
/// or reallocates the backing Vec.
#define heap_replace_top(T, self, value) RK__HEAP_F(T, replace_top)(self, value)

/// @brief `void heap_extend(T, Heap(T)* self, const T* arr, size_t n)` - Appends `arr`'s first `n`
/// values to the Heap's existing contents, then re-heapifies the combined set in O(count + n).
/// @param T   Element type
/// @param arr Source array of `n` values
/// @param n   Number of values to append
/// @note Prefer this over `n` individual `heap_push()` calls when `n` is a significant fraction of
/// the Heap's existing size; for a handful of new elements into an already-large Heap, looped
/// `heap_push()` (O(n log count)) stays cheaper than re-heapifying everything (O(count + n)).
/// @attention `arr[0..n)` must not overlap the Heap's own backing allocation, for the same reasons
/// documented on `heap_assign()`.
#define heap_extend(T, self, arr, n)     RK__HEAP_F(T, extend)((self), (arr), (n))

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RK__HEAP_F(T, NAME)              rk_heapf_##NAME##_##T

#define RK__HEAP_DEFINE(T, CMP_FUN)                                                                \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct Heap(T) { Vec(T) data; } Heap(T);                                                 \
  static_fun const T* RK__HEAP_F(T, peek)(const Heap(T) * self) {                                  \
    return vec_count(self->data) ? &self->data[0] : rk_null;                                       \
  }                                                                                                \
  static_fun void RK__HEAP_F(T, sift_down)(Heap(T) * self, size_t i, T value, size_t n) {          \
    while (i < n / 2) {                                                                            \
      size_t child = 2 * i + 1;                                                                    \
      if (child + 1 < n && CMP_FUN(self->data[child + 1], self->data[child]) < 0) { ++child; }     \
      if (CMP_FUN(value, self->data[child]) <= 0) { break; }                                       \
      self->data[i] = self->data[child];                                                           \
      i             = child;                                                                       \
    }                                                                                              \
    self->data[i] = value;                                                                         \
  }                                                                                                \
  static_fun void RK__HEAP_F(T, heapify)(Heap(T) * self) {                                         \
    const size_t n = vec_count(self->data);                                                        \
    for (size_t i = n / 2; i > 0;) {                                                               \
      --i;                                                                                         \
      RK__HEAP_F(T, sift_down)(self, i, self->data[i], n);                                         \
    }                                                                                              \
  }                                                                                                \
  static_fun Heap(T) RK__HEAP_F(T, from)(const T* arr, size_t n RK_IFALLOC(, Allocator alloc)) {   \
    Heap(T) h = heap_init(T, n RK_IFALLOC(, alloc));                                               \
    vec_push_n(h.data, arr, n);                                                                    \
    RK__HEAP_F(T, heapify)(&h);                                                                    \
    return h;                                                                                      \
  }                                                                                                \
  static_fun Heap(T) RK__HEAP_F(T, adopt)(Vec(T) vec) {                                            \
    Heap(T) h = {vec};                                                                             \
    RK__HEAP_F(T, heapify)(&h);                                                                    \
    return h;                                                                                      \
  }                                                                                                \
  static_fun void RK__HEAP_F(T, assign)(Heap(T) * self, const T* arr, size_t n) {                  \
    vec_assign(self->data, arr, n);                                                                \
    RK__HEAP_F(T, heapify)(self);                                                                  \
  }                                                                                                \
  static_fun void RK__HEAP_F(T, extend)(Heap(T) * self, const T* arr, size_t n) {                  \
    vec_push_n(self->data, arr, n);                                                                \
    RK__HEAP_F(T, heapify)(self);                                                                  \
  }                                                                                                \
  static_fun void RK__HEAP_F(T, push)(Heap(T) * self, T value) {                                   \
    vec_push(self->data, value);                                                                   \
    size_t i = vec_count(self->data) - 1;                                                          \
    while (i > 0) {                                                                                \
      size_t parent = (i - 1) / 2;                                                                 \
      if (CMP_FUN(value, self->data[parent]) >= 0) { break; }                                      \
      self->data[i] = self->data[parent];                                                          \
      i             = parent;                                                                      \
    }                                                                                              \
    self->data[i] = value;                                                                         \
  }                                                                                                \
  static_fun T RK__HEAP_F(T, pop)(Heap(T) * self) {                                                \
    rk_assert(vec_count(self->data) && "Cannot pop an empty heap");                                \
    const T      result = self->data[0], last = vec_pop(self->data);                               \
    const size_t n = vec_count(self->data);                                                        \
    if (n) { RK__HEAP_F(T, sift_down)(self, 0, last, n); }                                         \
    return result;                                                                                 \
  }                                                                                                \
  static_fun bool RK__HEAP_F(T, try_pop)(Heap(T) * self, T * out) {                                \
    if (!vec_count(self->data)) { return false; }                                                  \
    return *out = RK__HEAP_F(T, pop)(self), true;                                                  \
  }                                                                                                \
  static_fun T RK__HEAP_F(T, replace_top)(Heap(T) * self, T value) {                               \
    rk_assert(vec_count(self->data) && "Cannot replace_top an empty heap");                        \
    const T result = self->data[0];                                                                \
    RK__HEAP_F(T, sift_down)(self, 0, value, vec_count(self->data));                               \
    return result;                                                                                 \
  }                                                                                                \
  RK_EXTERNC_END

#define RK__HEAP_FROM(T, arr, n, alloc)  RK__HEAP_F(T, from)((arr), (n)RK_IFALLOC(, (alloc)))
#define RK__HEAP_FROM4(T, arr, n, alloc) rk_disable_if(RK__HEAP_FROM(T, arr, n, alloc))
#define RK__HEAP_FROM3(T, arr, n)        RK__HEAP_FROM(T, arr, n, alloc_ctx)

RK_HEADER_END

/// @}
#endif // RK_HEAP_H

// MIT License
//
// Copyright (c) 2026 Dariusch Knigge
//
// Permission is hereby granted, free of charge, to any person
// obtaining a copy of this software and associated documentation
// files (the "Software"), to deal in the Software without
// restriction, including without limitation the rights to use,
// copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following
// conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
// OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
// HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
// WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
