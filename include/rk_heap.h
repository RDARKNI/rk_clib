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
RKI_HEADER_BEGIN

/// @brief Define a heap type and functions for a given element type.
///
/// This macro generates a complete, type-specific binary min-heap API for the given element type.
///
/// @param T       Name of the element type. Must be an identifier; use a typedef for a pointer or
///                struct type.
/// @param CMP_FUN Comparison function (`int CMP_FUN(T a, T b)`), returning negative, zero, or
///                positive, following the same convention as `strcmp` and the tree comparators.
/// @attention Must be invoked at file scope, once per `T`.
#define HEAP_DEFINE(T, CMP_FUN)          RKI_HEAP_DEFINE(T, CMP_FUN)

/// @brief Macro to indicate that an object is a Heap.
/// @param T The type of elements stored in the Heap
#define Heap(T)                          Heap_##T

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
#define heap_from(T, arr, n, ...)        RKI_OVERLOAD(RKI_HEAP_FROM, T, arr, n, ##__VA_ARGS__)

/// @brief `Heap(T) heap_adopt(T, Vec(T) vec)` - Constructs a new Heap by taking ownership of `vec`
/// and heapifying it in place, with no copy.
/// @param T   Element type
/// @param vec An existing `Vec(T)`, passed by value
/// @return A `Heap(T)` wrapping `vec`'s own storage, now in heap order
/// @attention `vec` is consumed: its storage now belongs to the returned Heap. Do not read, mutate,
/// or `vec_release()` the original `vec` variable afterwards; release the Heap instead.
#define heap_adopt(T, vec)               RKI_HEAP_PUB(T, adopt)(vec)

/// @brief `void heap_release(Heap(T)* self)` - Frees the backing Vec and resets the Heap to an
/// empty state.
#define heap_release(self)               vec_release((self)->data)

/// @brief `size_t heap_count(const Heap(T)* self)` - Returns the number of elements in the Heap.
#define heap_count(self)                 vec_count((self)->data)

/// @brief `size_t heap_cap(const Heap(T)* self)` - Returns the current capacity of the backing Vec.
#define heap_cap(self)                   vec_cap((self)->data)

/// @brief `Allocator heap_allocator(const Heap(T)* self)` - Returns the Allocator the Heap's
/// backing Vec was constructed with, or `alloc_ctx` if the Heap was never initialized or custom
/// allocators are disabled.
#define heap_allocator(self)             vec_allocator((self)->data)

/// @brief `bool heap_is_empty(const Heap(T)* self)` - Returns `true` iff the Heap contains no
/// elements.
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
#define heap_assign(T, self, arr, n)     RKI_HEAP_PUB(T, assign)(self, arr, n)

/// @brief `const T* heap_peek(T, const Heap(T)* self)` - Returns a pointer to the minimum element
/// without removing it.
/// @param T Element type
/// @return Pointer to the minimum element, or `NULL` if the Heap is empty
/// @note Invalidated by any later mutation of the Heap.
#define heap_peek(T, self)               RKI_HEAP_PUB(T, peek)(self)

/// @brief Returns the top element according to the heap's ordering as a const lvalue.
/// @param T Element type.
/// @pre The Heap is nonempty; use `heap_peek()` to check safely.
/// @note Always const, even for a mutable Heap: writing the top in place would break the heap
/// order. Use `heap_replace_top()` to change it.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Invalidated by any later mutation of the Heap.
/// @see heap_peek
#define heap_top(T, self)                (*RKI_HEAP_PUB(T, top_ptr)(self))

/// @brief `void heap_push(T, Heap(T)* self, T value)` - Inserts `value` into the Heap.
/// @param T     Element type
/// @param value Value to insert. Evaluated once.
/// @note A push may reallocate the backing Vec, invalidating prior pointers into it.
#define heap_push(T, self, value)        RKI_HEAP_PUB(T, push)(self, value)

/// @brief `T heap_pop(T, Heap(T)* self)` - Removes and returns the minimum element.
/// @param T Element type
/// @return The (former) minimum element
/// @pre The Heap is nonempty; use `heap_try_pop()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
#define heap_pop(T, self)                RKI_HEAP_PUB(T, pop)(self)

/// @brief `bool heap_try_pop(T, Heap(T)* self, T* out)` - Removes the minimum element and writes
/// it to `*out`, if the Heap is nonempty.
/// @param T   Element type
/// @param out Destination for the removed value. Left untouched if the Heap is empty.
/// @return `true` if an element was removed, `false` if the Heap was empty
#define heap_try_pop(T, self, out)       RKI_HEAP_PUB(T, try_pop)(self, out)

/// @brief `T heap_replace_top(T, Heap(T)* self, T value)` - Removes the minimum element and
/// inserts `value`, in a single sift-down.
/// @param T     Element type
/// @param value Value to insert in place of the removed minimum. Evaluated once.
/// @return The (former) minimum element
/// @pre The Heap is nonempty.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Equivalent to, but cheaper than, `heap_pop()` followed by `heap_push()`: it never shrinks
/// or reallocates the backing Vec.
#define heap_replace_top(T, self, value) RKI_HEAP_PUB(T, replace_top)(self, value)

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
#define heap_extend(T, self, arr, n)     RKI_HEAP_PUB(T, extend)(self, arr, n)

/// @brief Visits every element in backing-array order.
///
/// A Heap's only real contract is the heap invariant (min/max at the root, O(log n)
/// push/pop/replace_top) -- the backing array's exact layout beyond that is an implementation
/// detail of how elements landed there via sift-up/sift-down, not a property the caller can rely
/// on. There is deliberately no `heap_foreach_reversed`: reversing an order that was never part of
/// the contract (same reasoning as `pool_foreach` having no `_reversed`, and Dict/Set's
/// "unspecified slot order" never getting one either) wouldn't add anything over calling this.
/// @param self Pointer to the Heap. Evaluated once.
/// @param it   Iterator name. Pointer to a const element.
/// @note break stops traversal; continue advances to the next element.
/// @note Do not modify the Heap during traversal.
///
/// Usage:
/// ```c
/// heap_foreach(&h, it) { printf("%d\n", *it); }
/// ```
#define heap_foreach(self, it)           vec_foreach((const typeof(*(self)->data)*)(self)->data, it)

/// @brief Erases every element satisfying `pred`, then restores the heap invariant.
/// @param T    Element type.
/// @param self Pointer to the mutable Heap. Evaluated once.
/// @param it   Iterator name (access via `*it`).
/// @param pred Predicate expression, evaluated once per original element.
/// @note The predicate must not structurally modify the Heap.
/// @note Takes O(n) time and does not allocate.
///
/// Usage:
/// ```c
/// heap_erase_if(int, &h, it, *it % 2 == 0); // drop even values
/// ```
#define heap_erase_if(T, self, it, pred) RKI_HEAP_ERASE_IF(T, self, it, pred)

#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RKI_HEAP_PUB(T, FNAME) heap_##T##_##FNAME
#define RKI_HEAP_PRI(T, FNAME) rki_heap_##T##_##FNAME

#define RKI_HEAP_DEFINE(T, CMP_FUN)                                                                \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct Heap(T) { Vec(T) data; } Heap(T);                                                 \
  rklib_fun rk_pure size_t RKI_HEAP_PUB(T, count)(const Heap(T) * self) {                          \
    return vec_count(self->data);                                                                  \
  }                                                                                                \
  rklib_fun rk_pure size_t RKI_HEAP_PUB(T, cap)(const Heap(T) * self) {                            \
    return vec_cap(self->data);                                                                    \
  }                                                                                                \
  rklib_fun rk_pure bool RKI_HEAP_PUB(T, is_empty)(const Heap(T) * self) {                         \
    return vec_count(self->data) == 0;                                                             \
  }                                                                                                \
  rklib_fun rk_pure Allocator RKI_HEAP_PUB(T, allocator)(const Heap(T) * self) {                   \
    return vec_allocator(self->data);                                                              \
  }                                                                                                \
  rklib_fun const T* RKI_HEAP_PUB(T, top_ptr)(const Heap(T) * self) {                              \
    rk_assert(heap_count(self) && "Cannot access top of empty heap");                              \
    return self->data;                                                                             \
  }                                                                                                \
  rklib_fun rk_pure const T* RKI_HEAP_PUB(T, peek)(const Heap(T) * self) {                         \
    return vec_count(self->data) ? &self->data[0] : rk_null;                                       \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PUB(T, release)(Heap(T) * self) { vec_release(self->data); }             \
  rklib_fun void RKI_HEAP_PUB(T, clear)(Heap(T) * self) { vec_clear(self->data); }                 \
  rklib_fun void RKI_HEAP_PUB(T, reserve)(Heap(T) * self, size_t cap) {                            \
    vec_reserve(self->data, cap);                                                                  \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PUB(T, shrink_to_fit)(Heap(T) * self) { vec_shrink_to_fit(self->data); } \
  rklib_fun void RKI_HEAP_PRI(T, sift_down)(Heap(T) * self, size_t i, T value, size_t n) {         \
    while (i < n / 2) {                                                                            \
      size_t child = 2 * i + 1;                                                                    \
      if (child + 1 < n && CMP_FUN(self->data[child + 1], self->data[child]) < 0) { ++child; }     \
      if (CMP_FUN(value, self->data[child]) <= 0) { break; }                                       \
      self->data[i] = self->data[child];                                                           \
      i             = child;                                                                       \
    }                                                                                              \
    self->data[i] = value;                                                                         \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PRI(T, heapify)(Heap(T) * self) {                                        \
    const size_t n = vec_count(self->data);                                                        \
    for (size_t i = n / 2; i > 0;) {                                                               \
      --i;                                                                                         \
      RKI_HEAP_PRI(T, sift_down)(self, i, self->data[i], n);                                       \
    }                                                                                              \
  }                                                                                                \
  rklib_fun Heap(T) RKI_HEAP_PUB(T, from)(const T* arr, size_t n RK_IFALLOC(, Allocator alloc)) {  \
    Heap(T) h = heap_init(T, n RK_IFALLOC(, alloc));                                               \
    vec_push_n(h.data, arr, n);                                                                    \
    RKI_HEAP_PRI(T, heapify)(&h);                                                                  \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun Heap(T) RKI_HEAP_PUB(T, adopt)(Vec(T) vec) {                                           \
    Heap(T) h = {vec};                                                                             \
    RKI_HEAP_PRI(T, heapify)(&h);                                                                  \
    return h;                                                                                      \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PUB(T, assign)(Heap(T) * self, const T* arr, size_t n) {                 \
    vec_assign(self->data, arr, n);                                                                \
    RKI_HEAP_PRI(T, heapify)(self);                                                                \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PUB(T, extend)(Heap(T) * self, const T* arr, size_t n) {                 \
    vec_push_n(self->data, arr, n);                                                                \
    RKI_HEAP_PRI(T, heapify)(self);                                                                \
  }                                                                                                \
  rklib_fun void RKI_HEAP_PUB(T, push)(Heap(T) * self, T value) {                                  \
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
  rklib_fun T RKI_HEAP_PUB(T, pop)(Heap(T) * self) {                                               \
    rk_assert(vec_count(self->data) && "Cannot pop an empty heap");                                \
    const T      result = self->data[0], last = vec_pop(self->data);                               \
    const size_t n = vec_count(self->data);                                                        \
    if (n) { RKI_HEAP_PRI(T, sift_down)(self, 0, last, n); }                                       \
    return result;                                                                                 \
  }                                                                                                \
  rklib_fun bool RKI_HEAP_PUB(T, try_pop)(Heap(T) * self, T * out) {                               \
    if (!vec_count(self->data)) { return false; }                                                  \
    return *out = RKI_HEAP_PUB(T, pop)(self), true;                                                \
  }                                                                                                \
  rklib_fun T RKI_HEAP_PUB(T, replace_top)(Heap(T) * self, T value) {                              \
    rk_assert(vec_count(self->data) && "Cannot replace_top an empty heap");                        \
    const T result = self->data[0];                                                                \
    RKI_HEAP_PRI(T, sift_down)(self, 0, value, vec_count(self->data));                             \
    return result;                                                                                 \
  }                                                                                                \
  RK_EXTERNC_END

#define RKI_HEAP_FROM(T, arr, n, alloc) RKI_HEAP_PUB(T, from)((arr), (n)RK_IFALLOC(, (alloc)))
#define RKI_HEAP_FROM4(T, arr, n, alloc)                                                           \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_HEAP_FROM(T, arr, n, alloc))
#define RKI_HEAP_FROM3(T, arr, n) RKI_HEAP_FROM(T, arr, n, alloc_ctx)

#define RKI_HEAP_ERASE_IF(T, self, it, pred)                                                       \
  do {                                                                                             \
    Heap(T)* const rki_var_heap = (self);                                                          \
    if (!rki_var_heap) { break; }                                                                  \
    const size_t rki_var_count = heap_count(rki_var_heap);                                         \
    vec_erase_if(rki_var_heap->data, it, pred);                                                    \
    if (heap_count(rki_var_heap) != rki_var_count) { RKI_HEAP_PRI(T, heapify)(rki_var_heap); }     \
  } while (0)

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
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
