// SPDX-License-Identifier: MIT
/// @file rk_deque.h
/// @version 1.0.0
/// @defgroup rk_deque Deque (Double-Ended Queue) Interface
/// @brief Type-specific circular-buffer deque with amortized O(1) insertion and removal at either
/// end.
///
/// Define a specialization once at file scope with `DEQUE_DEFINE(T)`.
///
/// Usage:
/// ```c
/// DEQUE_DEFINE(int);
/// Deque(int) q = deque_init(int, 0); // optional initial capacity and allocator
/// deque_push_back(int, &q, 1);
/// deque_push_front(int, &q, 2);
/// int first = deque_pop_front(int, &q); // 2
/// deque_release(int, &q);
/// ```
///
/// Indices count from the front, not from the beginning of the allocation: the backing array is a
/// power-of-two-sized ring buffer, so its physical order does not necessarily match logical order
/// once it has wrapped. As with `Vec`, insertion or `deque_reserve()` can invalidate pointers into
/// the Deque.
/// @note If only one end is ever pushed/popped, `Vec` is simpler and has no head/wraparound
/// bookkeeping at all; reach for `Deque` specifically when both ends are needed.
/// @see rk_vec.h
/// @{
#ifndef RK_DEQUE_H
#define RK_DEQUE_H
#include "rk_alloc.h"
RK_HEADER_BEGIN

/// @brief Generates a deque type and its operations for T. Invoke once per T at file scope.
/// @param T Name of the element type. Must be a plain type identifier; use a typedef for a pointer
/// or struct type.
/// @attention Must be invoked at file scope, once per `T`.
/// @note Allocator functions handle allocation failures according to `rk_alloc.h`.
#define DEQUE_DEFINE(T)            RK__DEQUE_DEFINE(T)

/// @brief Macro to indicate that an object is a Deque.
/// @param T The type of elements stored in the Deque
#define Deque(T)                   rk_deque_##T

/// @brief `Deque(T) deque_init(T, size_t capacity, Allocator alloc = alloc_ctx)` - Initialises and
/// returns an empty Deque.
/// @param T        The desired type of the Deque's elements
/// @param capacity The initial capacity of the backing buffer (in elements)
/// @param alloc    Optional allocator; defaults to `alloc_ctx`
/// @return An initialised, empty `Deque(T)`
/// @note A zero-initialized `Deque(T)` is also a valid, empty deque; it allocates using `alloc_ctx`
/// on first insertion.
#define deque_init(T, cap, ...)    rk_overload(RK__DEQUE_INIT, T, cap, ##__VA_ARGS__)

/// @brief `Deque(T) deque_from(T, const T* arr, size_t n, Allocator alloc = alloc_ctx)` -
/// Constructs a new Deque by copying `n` values from `arr`, in front-to-back order.
/// @param T     Element type
/// @param arr   Source array of `n` values
/// @param n     Number of values to copy
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return A new `Deque(T)` containing a copy of `arr`'s first `n` values
#define deque_from(T, arr, n, ...) rk_overload(RK__DEQUE_FROM, T, arr, n, ##__VA_ARGS__)

/// @brief `void deque_release(T, Deque(T)* self)` - Frees the backing buffer and resets the Deque
/// to an empty state.
/// @param T Element type
/// @note Safe to call on a zero-initialized Deque.
#define deque_release(T, self)     RK__DEQUE_F(T, release)(self)

/// @brief `size_t deque_count(Deque(T)* self)` - Returns the number of elements stored in the
/// Deque.
#define deque_count(self)          ((size_t)(self)->count)

/// @brief `size_t deque_cap(Deque(T)* self)` - Returns the current capacity of the backing buffer.
/// Always a power of two (or zero).
#define deque_cap(self)            ((size_t)(self)->cap)

#if RK_CUSTOM_ALLOCATORS
/// @brief `Allocator deque_allocator(Deque(T)* self)` - Returns the Allocator the Deque was
/// constructed with.
# define deque_allocator(self) rk_to_rvalue((self)->alloc)
#else
/// @brief `Allocator deque_allocator(Deque(T)* self)` - Returns `alloc_ctx` (allocators disabled).
# define deque_allocator(self) ((void)(self), alloc_ctx)
#endif

/// @brief `bool deque_is_empty(Deque(T)* self)` - Returns `true` iff the Deque contains no
/// elements.
#define deque_is_empty(self)          (deque_count(self) == 0)

/// @brief `void deque_clear(T, Deque(T)* self)` - Removes all elements without freeing the backing
/// buffer.
/// @param T Element type
#define deque_clear(T, self)          RK__DEQUE_F(T, clear)(self)

/// @brief `void deque_reserve(T, Deque(T)* self, size_t cap)` - Ensures the backing buffer holds at
/// least `cap` elements without reallocating.
/// @param T   Element type
/// @param cap Minimum capacity to reserve (in elements)
/// @note Existing elements retain their logical order.
#define deque_reserve(T, self, cap)   RK__DEQUE_F(T, reserve)((self), (cap))

/// @brief `void deque_shrink_to_fit(T, Deque(T)* self)` - Shrinks the Deque's capacity to the next
/// power of two greater than or equal to its length, with a floor of 8 for a nonempty Deque
/// (matching the same minimum `deque_init()`/`deque_reserve()` enforce), leaving contents
/// unchanged.
/// @param T Element type
/// @note Frees the backing buffer entirely if the Deque is empty.
#define deque_shrink_to_fit(T, self)  RK__DEQUE_F(T, shrink_to_fit)(self)

/// @brief `void deque_assign(T, Deque(T)* self, const T* arr, size_t n)` - Replaces the Deque's
/// contents with a copy of `arr`'s first `n` values, reusing the existing backing buffer (growing
/// it if necessary) rather than allocating a new one.
/// @param T   Element type
/// @param arr Source array of `n` values
/// @param n   Number of values to copy
/// @attention `arr[0..n)` must not overlap the Deque's own backing allocation, for the same reasons
/// documented on `deque_push_back_n()`.
#define deque_assign(T, self, arr, n) RK__DEQUE_F(T, assign)((self), (arr), (n))

/// @brief Returns the first element as an lvalue, mutable for `Deque(T)* self` and const for
/// `const Deque(T)* self`. Like `vec_front()`, requires a nonempty Deque.
/// @param T Element type
/// @return Lvalue for the first element
/// @attention Requires a nonempty Deque; use `deque_peek_front()` to check safely.
/// @note Invalidated by any later mutation of the Deque.
#define deque_front(T, self)                                                                       \
  (*_Generic((self),                                                                               \
       const Deque(T)*: RK__DEQUE_F(T, front_const),                                               \
       default: RK__DEQUE_F(T, front))(self))

/// @brief Returns the last element as an lvalue, mutable for `Deque(T)* self` and const for
/// `const Deque(T)* self`. Like `vec_back()`, requires a nonempty Deque.
/// @param T Element type
/// @return Lvalue for the last element
/// @attention Requires a nonempty Deque; use `deque_peek_back()` to check safely.
/// @note Invalidated by any later mutation of the Deque.
#define deque_back(T, self)                                                                        \
  (*_Generic((self), const Deque(T)*: RK__DEQUE_F(T, back_const), default: RK__DEQUE_F(T, back))(  \
      self))

/// @brief Returns a pointer to the element at the zero-based logical index (counting from the
/// front): `T*` for `Deque(T)* self`, `const T*` for `const Deque(T)* self`.
/// @param T     Element type
/// @param index Zero-based logical index
/// @return Pointer to the element, or `NULL` if `index` is out of bounds
/// @note Invalidated by any later mutation of the Deque.
#define deque_at(T, self, index)                                                                   \
  _Generic((self), const Deque(T)*: RK__DEQUE_F(T, at_const), default: RK__DEQUE_F(T, at))(        \
      (self), (index))

/// @brief Returns a pointer to the first element: `T*` for `Deque(T)* self`, `const T*` for
/// `const Deque(T)* self`.
/// @param T Element type
/// @return Pointer to the first element, or `NULL` if the Deque is empty
/// @note Invalidated by any later mutation of the Deque.
#define deque_peek_front(T, self)                                                                  \
  _Generic((self),                                                                                 \
      const Deque(T)*: RK__DEQUE_F(T, peek_front_const),                                           \
      default: RK__DEQUE_F(T, peek_front))(self)

/// @brief Returns a pointer to the last element: `T*` for `Deque(T)* self`, `const T*` for
/// `const Deque(T)* self`.
/// @param T Element type
/// @return Pointer to the last element, or `NULL` if the Deque is empty
/// @note Invalidated by any later mutation of the Deque.
#define deque_peek_back(T, self)                                                                   \
  _Generic((self),                                                                                 \
      const Deque(T)*: RK__DEQUE_F(T, peek_back_const),                                            \
      default: RK__DEQUE_F(T, peek_back))(self)

/// @brief `void deque_push_front(T, Deque(T)* self, T value)` - Inserts `value` at the front of the
/// Deque.
/// @param T     Element type
/// @param value Value to insert. Evaluated once.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
#define deque_push_front(T, self, value)        RK__DEQUE_F(T, push_front)((self), (value))

/// @brief `void deque_push_front_n(T, Deque(T)* self, const T* arr, size_t count)` - Prepends
/// `count` values from `arr` to the front of the Deque, preserving `arr`'s own order (`arr[0]`
/// becomes the new first element).
/// @param T     Element type
/// @param arr   Source array of `count` values
/// @param count Number of values to prepend
/// @note This is NOT equivalent to `count` individual `deque_push_front()` calls, which would
/// insert them in reverse order; `arr`'s order is preserved instead, matching
/// `vec_insert_arr_at()`'s convention. Reserves once and copies in at most two segments, instead of
/// re-checking capacity per element.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
/// @attention `arr[0..count)` must not overlap the Deque's own backing allocation. If growth is
/// triggered, the old buffer is freed before the copy from `arr` happens, turning an `arr` that
/// points into it into a use-after-free; even without growth, the underlying copy is a plain
/// `memcpy`, which is undefined for overlapping source and destination. To insert elements taken
/// from the same Deque, copy them into a temporary buffer first.
#define deque_push_front_n(T, self, arr, count) RK__DEQUE_F(T, push_front_n)((self), (arr), (count))

/// @brief `void deque_push_back(T, Deque(T)* self, T value)` - Inserts `value` at the back of the
/// Deque.
/// @param T     Element type
/// @param value Value to insert. Evaluated once.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
#define deque_push_back(T, self, value)         RK__DEQUE_F(T, push_back)((self), (value))

/// @brief `void deque_push_back_n(T, Deque(T)* self, const T* arr, size_t count)` - Appends `count`
/// values from `arr` to the back of the Deque, in order, as a single bulk operation.
/// @param T     Element type
/// @param arr   Source array of `count` values
/// @param count Number of values to append
/// @note Reserves once and copies in at most two segments, instead of re-checking capacity per
/// element like `count` individual `deque_push_back()` calls would.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
/// @attention `arr[0..count)` must not overlap the Deque's own backing allocation, for the same
/// reasons documented on `deque_push_front_n()`.
#define deque_push_back_n(T, self, arr, count)  RK__DEQUE_F(T, push_back_n)((self), (arr), (count))

/// @brief `T deque_pop_front(T, Deque(T)* self)` - Removes and returns the first element.
/// @param T Element type
/// @return The (former) first element
/// @attention Requires a nonempty Deque.
#define deque_pop_front(T, self)                RK__DEQUE_F(T, pop_front)(self)

/// @brief `T deque_pop_back(T, Deque(T)* self)` - Removes and returns the last element.
/// @param T Element type
/// @return The (former) last element
/// @attention Requires a nonempty Deque.
#define deque_pop_back(T, self)                 RK__DEQUE_F(T, pop_back)(self)

/// @brief `bool deque_try_pop_front(T, Deque(T)* self, T* out)` - Removes the first element and
/// writes it to `*out`, if the Deque is nonempty.
/// @param T   Element type
/// @param out Destination for the removed value. Left untouched if the Deque is empty.
/// @return `true` if an element was removed, `false` if the Deque was empty
#define deque_try_pop_front(T, self, out)       RK__DEQUE_F(T, try_pop_front)((self), (out))

/// @brief `bool deque_try_pop_back(T, Deque(T)* self, T* out)` - Removes the last element and
/// writes it to `*out`, if the Deque is nonempty.
/// @param T   Element type
/// @param out Destination for the removed value. Left untouched if the Deque is empty.
/// @return `true` if an element was removed, `false` if the Deque was empty
#define deque_try_pop_back(T, self, out)        RK__DEQUE_F(T, try_pop_back)((self), (out))

/// @brief Visits every element of a Deque in front-to-back order.
/// @param self The Deque to loop over (a pointer). Evaluated once.
/// @param it   The name of the iterator (pointer to each element, const if `self` points to a
///             const Deque)
/// @note Do not push, pop, reserve, or shrink the Deque while looping in this fashion.
///
/// Usage:
/// ```c
/// deque_foreach(&q, it) { printf("%d\n", *it); }
/// ```
#define deque_foreach(self, it)                                                                    \
  for (typeof(self) RK___DEQUE = (self); RK___DEQUE; RK___DEQUE = rk_null)                         \
    for (size_t RK___i = 0; RK___i < RK___DEQUE->count; ++RK___i)                                  \
      for (typeof(RK__DEQUE_ITER_PTR(RK___DEQUE)) it                                               \
           = &RK___DEQUE->data[(RK___DEQUE->head + RK___i) & (RK___DEQUE->cap - 1)],               \
           RK___once            = it;                                                              \
           RK___once; RK___once = rk_null)

/// @brief Like `deque_foreach()`, visiting elements in back-to-front order. Iterator element
/// constness follows the constness of the Deque pointed to by `self`.
#define deque_foreach_reversed(self, it)                                                           \
  for (typeof(self) RK___DEQUE = (self); RK___DEQUE; RK___DEQUE = rk_null)                         \
    for (size_t RK___i = RK___DEQUE->count; RK___i-- > 0;)                                         \
      for (typeof(RK__DEQUE_ITER_PTR(RK___DEQUE)) it                                               \
           = &RK___DEQUE->data[(RK___DEQUE->head + RK___i) & (RK___DEQUE->cap - 1)],               \
           RK___once            = it;                                                              \
           RK___once; RK___once = rk_null)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL
#define RK__DEQUE_F(T, NAME) rk_dequef_##NAME##_##T
// The member `data` is a mutable pointer even when the Deque is const; propagate the
// container's constness explicitly when choosing an iterator pointer type.
#define RK__DEQUE_ITER_PTR(self)                                                                   \
  _Generic((self),                                                                                 \
      const typeof(*(self))*: (const typeof((self)->data[0])*)0,                                   \
      default: (typeof((self)->data))0)
#define RK__DEQUE_DEFINE(T)                                                                        \
  RK_EXTERNC_BEG                                                                                   \
  typedef struct Deque(T) {                                                                        \
    T*     data;                                                                                   \
    size_t head, count, cap;                                                                       \
    RK_IFALLOC(Allocator alloc;)                                                                   \
  } Deque(T);                                                                                      \
  static_fun Deque(T) RK__DEQUE_F(T, init)(size_t cap RK_IFALLOC(, Allocator alloc)) {             \
    RK_IFALLOC(rk_assert_allocator_valid(alloc);)                                                  \
    Deque(T) result = {rk_null, 0, 0, 0 RK_IFALLOC(, alloc)};                                      \
    if (cap) {                                                                                     \
      cap = stdc_bit_ceil(rk_MAX((size_t)8, cap));                                                 \
      rk_assert(cap && "Deque capacity overflow");                                                 \
      result.data = alloc_new(T, cap RK_IFALLOC(, alloc));                                         \
      result.cap  = cap;                                                                           \
    }                                                                                              \
    return result;                                                                                 \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, release)(Deque(T) * self) {                                       \
    if (self->data) { alloc_delete(self->data, self->cap RK_IFALLOC(, self->alloc)); }             \
    self->data = rk_null, self->head = self->count = self->cap = 0;                                \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, clear)(Deque(T) * self) { self->head = self->count = 0; }         \
  static_fun void RK__DEQUE_F(T, realloc_to)(Deque(T) * self, size_t new_cap) {                    \
    T* data = alloc_new(T, new_cap RK_IFALLOC(, self->alloc));                                     \
    if (self->count) {                                                                             \
      size_t first = self->count < self->cap - self->head ? self->count : self->cap - self->head;  \
      rk_memcpy(data, self->data + self->head, sizeof_n(T, first));                                \
      if (first < self->count) {                                                                   \
        rk_memcpy(data + first, self->data, sizeof_n(T, self->count - first));                     \
      }                                                                                            \
    }                                                                                              \
    if (self->data) { alloc_delete(self->data, self->cap RK_IFALLOC(, self->alloc)); }             \
    self->data = data, self->head = 0, self->cap = new_cap;                                        \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, reserve)(Deque(T) * self, size_t requested) {                     \
    if (requested <= self->cap) { return; }                                                        \
    size_t cap = stdc_bit_ceil(rk_MAX((size_t)8, requested));                                      \
    rk_assert(cap && "Deque capacity overflow");                                                   \
    RK_IFALLOC(rk_set_alloc_fallback(self->alloc);)                                                \
    RK__DEQUE_F(T, realloc_to)(self, cap);                                                         \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, shrink_to_fit)(Deque(T) * self) {                                 \
    if (!self->count) {                                                                            \
      RK__DEQUE_F(T, release)(self);                                                               \
      return;                                                                                      \
    }                                                                                              \
    size_t cap = stdc_bit_ceil(rk_MAX((size_t)8, self->count));                                    \
    if (cap == self->cap) { return; }                                                              \
    RK_IFALLOC(rk_set_alloc_fallback(self->alloc);)                                                \
    RK__DEQUE_F(T, realloc_to)(self, cap);                                                         \
  }                                                                                                \
  static_fun T* RK__DEQUE_F(T, at)(Deque(T) * self, size_t i) {                                    \
    return i < self->count ? &self->data[(self->head + i) & (self->cap - 1)] : rk_null;            \
  }                                                                                                \
  static_fun const T* RK__DEQUE_F(T, at_const)(const Deque(T) * self, size_t i) {                  \
    return i < self->count ? &self->data[(self->head + i) & (self->cap - 1)] : rk_null;            \
  }                                                                                                \
  static_fun T* RK__DEQUE_F(T, peek_front)(Deque(T) * self) {                                      \
    return RK__DEQUE_F(T, at)(self, 0);                                                            \
  }                                                                                                \
  static_fun const T* RK__DEQUE_F(T, peek_front_const)(const Deque(T) * self) {                    \
    return RK__DEQUE_F(T, at_const)(self, 0);                                                      \
  }                                                                                                \
  static_fun T* RK__DEQUE_F(T, peek_back)(Deque(T) * self) {                                       \
    return self->count ? RK__DEQUE_F(T, at)(self, self->count - 1) : rk_null;                      \
  }                                                                                                \
  static_fun T const* RK__DEQUE_F(T, peek_back_const)(const Deque(T) * self) {                     \
    return self->count ? RK__DEQUE_F(T, at_const)(self, self->count - 1) : rk_null;                \
  }                                                                                                \
  static_fun T* RK__DEQUE_F(T, front)(Deque(T) * self) {                                           \
    rk_assert(self->count && "Cannot access front of empty deque");                                \
    return RK__DEQUE_F(T, at)(self, 0);                                                            \
  }                                                                                                \
  static_fun const T* RK__DEQUE_F(T, front_const)(const Deque(T) * self) {                         \
    rk_assert(self->count && "Cannot access front of empty deque");                                \
    return RK__DEQUE_F(T, at_const)(self, 0);                                                      \
  }                                                                                                \
  static_fun T* RK__DEQUE_F(T, back)(Deque(T) * self) {                                            \
    rk_assert(self->count && "Cannot access back of empty deque");                                 \
    return RK__DEQUE_F(T, at)(self, self->count - 1);                                              \
  }                                                                                                \
  static_fun const T* RK__DEQUE_F(T, back_const)(const Deque(T) * self) {                          \
    rk_assert(self->count && "Cannot access back of empty deque");                                 \
    return RK__DEQUE_F(T, at_const)(self, self->count - 1);                                        \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, push_front)(Deque(T) * self, T value) {                           \
    if (self->count == self->cap) {                                                                \
      rk_assert(self->cap <= SIZE_MAX / 2 && "Deque capacity overflow");                           \
      RK__DEQUE_F(T, reserve)(self, self->cap ? self->cap * 2 : 8);                                \
    }                                                                                              \
    self->head             = (self->head - 1) & (self->cap - 1);                                   \
    self->data[self->head] = value;                                                                \
    ++self->count;                                                                                 \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, push_back)(Deque(T) * self, T value) {                            \
    if (self->count == self->cap) {                                                                \
      rk_assert(self->cap <= SIZE_MAX / 2 && "Deque capacity overflow");                           \
      RK__DEQUE_F(T, reserve)(self, self->cap ? self->cap * 2 : 8);                                \
    }                                                                                              \
    self->data[(self->head + self->count) & (self->cap - 1)] = value;                              \
    ++self->count;                                                                                 \
  }                                                                                                \
  static_fun T RK__DEQUE_F(T, pop_front)(Deque(T) * self) {                                        \
    rk_assert(self->count && "Cannot pop an empty deque");                                         \
    T result   = self->data[self->head];                                                           \
    self->head = (self->head + 1) & (self->cap - 1);                                               \
    if (!--self->count) { self->head = 0; }                                                        \
    return result;                                                                                 \
  }                                                                                                \
  static_fun T RK__DEQUE_F(T, pop_back)(Deque(T) * self) {                                         \
    rk_assert(self->count && "Cannot pop an empty deque");                                         \
    T result = self->data[(self->head + self->count - 1) & (self->cap - 1)];                       \
    if (!--self->count) { self->head = 0; }                                                        \
    return result;                                                                                 \
  }                                                                                                \
  static_fun bool RK__DEQUE_F(T, try_pop_front)(Deque(T) * self, T * out) {                        \
    return self->count ? (*out = RK__DEQUE_F(T, pop_front)(self), true) : false;                   \
  }                                                                                                \
  static_fun bool RK__DEQUE_F(T, try_pop_back)(Deque(T) * self, T * out) {                         \
    return self->count ? (*out = RK__DEQUE_F(T, pop_back)(self), true) : false;                    \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, push_front_n)(Deque(T) * self, const T* arr, size_t n) {          \
    if (!n) { return; }                                                                            \
    rk_assert(n <= SIZE_MAX - self->count && "Deque capacity overflow");                           \
    RK__DEQUE_F(T, reserve)(self, self->count + n);                                                \
    self->head   = (self->head - n) & (self->cap - 1);                                             \
    size_t first = self->cap - self->head < n ? self->cap - self->head : n;                        \
    rk_memcpy(self->data + self->head, arr, sizeof_n(T, first));                                   \
    if (first < n) { rk_memcpy(self->data, arr + first, sizeof_n(T, n - first)); }                 \
    self->count += n;                                                                              \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, push_back_n)(Deque(T) * self, const T* arr, size_t n) {           \
    if (!n) { return; }                                                                            \
    rk_assert(n <= SIZE_MAX - self->count && "Deque capacity overflow");                           \
    RK__DEQUE_F(T, reserve)(self, self->count + n);                                                \
    size_t start = (self->head + self->count) & (self->cap - 1);                                   \
    size_t first = self->cap - start < n ? self->cap - start : n;                                  \
    rk_memcpy(self->data + start, arr, sizeof_n(T, first));                                        \
    if (first < n) { rk_memcpy(self->data, arr + first, sizeof_n(T, n - first)); }                 \
    self->count += n;                                                                              \
  }                                                                                                \
  static_fun void RK__DEQUE_F(T, assign)(Deque(T) * self, const T* arr, size_t n) {                \
    RK__DEQUE_F(T, clear)(self);                                                                   \
    RK__DEQUE_F(T, push_back_n)(self, arr, n);                                                     \
  }                                                                                                \
  static_fun Deque(T) RK__DEQUE_F(T, from)(const T* arr, size_t n RK_IFALLOC(, Allocator alloc)) { \
    Deque(T) d = RK__DEQUE_F(T, init)(n RK_IFALLOC(, alloc));                                      \
    RK__DEQUE_F(T, push_back_n)(&d, arr, n);                                                       \
    return d;                                                                                      \
  }                                                                                                \
  RK_EXTERNC_END

RK_HEADER_END

#define RK__DEQUE_INIT(T, cap, alloc)     RK__DEQUE_F(T, init)(cap RK_IFALLOC(, alloc))
#define RK__DEQUE_INIT3(T, cap, alloc)    rk_disable_if(RK__DEQUE_INIT(T, cap, alloc))
#define RK__DEQUE_INIT2(T, cap)           RK__DEQUE_INIT(T, cap, alloc_ctx)

#define RK__DEQUE_FROM(T, arr, n, alloc)  RK__DEQUE_F(T, from)((arr), (n)RK_IFALLOC(, (alloc)))
#define RK__DEQUE_FROM4(T, arr, n, alloc) rk_disable_if(RK__DEQUE_FROM(T, arr, n, alloc))
#define RK__DEQUE_FROM3(T, arr, n)        RK__DEQUE_FROM(T, arr, n, alloc_ctx)

/// @}
#endif // RK_DEQUE_H

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
