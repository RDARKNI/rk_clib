// SPDX-License-Identifier: MIT
/// @file rk_deque.h
/// @version 1.0.0
/// @defgroup rk_deque Deque (Double-Ended Queue) Interface
/// @brief Type-generic circular-buffer deque with amortized O(1) insertion and removal at either
/// end.
///
/// Declare the Deque type for an element type once at file scope with `DEQUE_DEFINE(T)`. Like
/// `Pool`, a Deque needs no type-specific behavior: every operation depends only on the element's
/// size and alignment, which the macros recover from the Deque itself. Only the constructors take
/// `T`, because they have to name the Deque type.
///
/// Usage:
/// ```c
/// DEQUE_DEFINE(int)
/// Deque(int) q = deque_init(int, 0); // optional initial capacity and allocator
/// deque_push_back(&q, 1);
/// deque_push_front(&q, 2);
/// int first = deque_pop_front(&q); // 2
/// deque_release(&q);
/// ```
///
/// Indices count from the front, not from the beginning of the allocation: the backing array is a
/// power-of-two-sized ring buffer, so its physical order does not necessarily match logical order
/// once it has wrapped. As with `Vec`, insertion or `deque_reserve()` can invalidate pointers into
/// the Deque.
/// @note `self` is evaluated exactly once by every macro taking it; element-type information is
/// only ever recovered from it inside `typeof`/`sizeof`, which do not evaluate it.
/// @note If only one end is ever pushed/popped, `Vec` is simpler and has no head/wraparound
/// bookkeeping at all; reach for `Deque` specifically when both ends are needed.
/// @see rk_vec.h
/// @see rk_pool.h
/// @{
#ifndef RK_DEQUE_H
#define RK_DEQUE_H
#include "rk_alloc.h"
RKI_HEADER_BEGIN

/// @brief Declares the Deque type for element type `T`. Invoke once per `T` at file scope.
/// @param T Name of the element type. Must be a plain type identifier; use a typedef for a pointer
/// or struct type.
/// @note Generates only the type; all operations are shared across element types.
#define DEQUE_DEFINE(T)            RKI_DEQUE_DEFINE(T)

/// @brief Macro to indicate that an object is a Deque.
/// @param T The type of elements stored in the Deque
#define Deque(T)                   Deque_##T

/// @brief `Deque(T) deque_init(T, size_t capacity, Allocator alloc = alloc_ctx)` - Initialises and
/// returns an empty Deque.
/// @param T        The desired type of the Deque's elements
/// @param capacity The initial capacity of the backing buffer (in elements); zero allocates nothing
/// @param alloc    Optional allocator; defaults to `alloc_ctx`
/// @return An initialised, empty `Deque(T)` bound to `alloc`, even for a capacity of zero.
/// @note A zero-initialized `Deque(T)` is also a valid, empty deque; it allocates using `alloc_ctx`
/// on first insertion.
#define deque_init(T, cap, ...)    RKI_OVERLOAD(RKI_DEQUE_INIT, T, cap, ##__VA_ARGS__)

/// @brief `Deque(T) deque_from(T, const T* arr, size_t n, Allocator alloc = alloc_ctx)` -
/// Constructs a new Deque by copying `n` values from `arr`, in front-to-back order.
/// @param T     Element type
/// @param arr   Source array of `n` values; must point to `T`
/// @param n     Number of values to copy
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return A new `Deque(T)` containing a copy of `arr`'s first `n` values
#define deque_from(T, arr, n, ...) RKI_OVERLOAD(RKI_DEQUE_FROM, T, arr, n, ##__VA_ARGS__)

/// @brief `void deque_release(Deque(T)* self)` - Frees the backing buffer and resets the Deque to
/// an empty state, keeping its allocator.
/// @note Safe to call on a zero-initialized Deque.
#define deque_release(self)        RKI_DEQUE_RELEASE(self)

/// @brief `size_t deque_count(const Deque(T)* self)` - Returns the number of elements stored in the
/// Deque.
#define deque_count(self)          ((size_t)(self)->base.count)

/// @brief `size_t deque_cap(const Deque(T)* self)` - Returns the current capacity of the backing
/// buffer. Always a power of two (or zero).
#define deque_cap(self)            ((size_t)(self)->base.cap)

/// @brief `Allocator deque_allocator(const Deque(T)* self)` - Returns the Allocator the Deque was
/// constructed with, or `alloc_ctx` if the Deque was never initialized or custom allocators are
/// disabled.
#define deque_allocator(self)      RKI_allocatorof(&(self)->base)

/// @brief `bool deque_is_empty(const Deque(T)* self)` - Returns `true` iff the Deque contains no
/// elements.
#define deque_is_empty(self)       ((bool)((self)->base.count == 0))

/// @brief `void deque_clear(Deque(T)* self)` - Removes all elements without freeing the backing
/// buffer.
#define deque_clear(self)          rki_deque_clear(&(self)->base)

/// @brief `void deque_reserve(Deque(T)* self, size_t cap)` - Ensures the backing buffer holds at
/// least `cap` elements without reallocating.
/// @param cap Minimum capacity to reserve (in elements)
/// @note Existing elements retain their logical order.
#define deque_reserve(self, cap)   RKI_DEQUE_RESERVE(self, cap)

/// @brief `void deque_shrink_to_fit(Deque(T)* self)` - Shrinks the Deque's capacity to the next
/// power of two greater than or equal to its length, with a floor of 8 for a nonempty Deque
/// (matching the same minimum `deque_init()`/`deque_reserve()` enforce), leaving contents
/// unchanged.
/// @note Frees the backing buffer entirely if the Deque is empty, keeping its allocator.
#define deque_shrink_to_fit(self)  RKI_DEQUE_SHRINK_TO_FIT(self)

/// @brief `void deque_assign(Deque(T)* self, const T* arr, size_t n)` - Replaces the Deque's
/// contents with a copy of `arr`'s first `n` values, reusing the existing backing buffer (growing
/// it if necessary) rather than allocating a new one.
/// @param arr Source array of `n` values; must point to the Deque's element type
/// @param n   Number of values to copy
/// @attention `arr[0..n)` must not overlap the Deque's own backing allocation, for the same reasons
/// documented on `deque_push_back_n()`.
#define deque_assign(self, arr, n) RKI_DEQUE_ASSIGN(self, arr, n)

/// @brief Returns the first element as an lvalue, mutable for `Deque(T)* self` and const for
/// `const Deque(T)* self`. Like `vec_front()`, requires a nonempty Deque.
/// @return Lvalue for the first element
/// @pre The Deque is nonempty; use `deque_peek_front()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Invalidated by any later mutation of the Deque.
#define deque_front(self) (*(RKI_DEQUE_ELEM_PTR(self))rki_deque_front(RKI_DEQUE_ARGS(self)))

/// @brief Returns the last element as an lvalue, mutable for `Deque(T)* self` and const for
/// `const Deque(T)* self`. Like `vec_back()`, requires a nonempty Deque.
/// @return Lvalue for the last element
/// @pre The Deque is nonempty; use `deque_peek_back()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Invalidated by any later mutation of the Deque.
#define deque_back(self)  (*(RKI_DEQUE_ELEM_PTR(self))rki_deque_back(RKI_DEQUE_ARGS(self)))

/// @brief Returns the element at the zero-based logical index (counting from the front) as an
/// lvalue, mutable for `Deque(T)* self` and const for `const Deque(T)* self`.
/// @param index Zero-based logical index
/// @pre `index < deque_count(self)`.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
/// @note Invalidated by any later mutation of the Deque.
/// @see deque_peek_at
#define deque_at(self, index)                                                                      \
  (*(RKI_DEQUE_ELEM_PTR(self))rki_deque_at(RKI_DEQUE_ARGS(self), (index)))

/// @brief Returns a pointer to the element at the zero-based logical index (counting from the
/// front): `T*` for `Deque(T)* self`, `const T*` for `const Deque(T)* self`.
/// @param index Zero-based logical index
/// @return Pointer to the element, or `NULL` if `index` is out of bounds (including an empty
/// Deque).
/// @note Bounds are checked in both debug and release builds.
/// @note Invalidated by any later mutation of the Deque.
#define deque_peek_at(self, index)                                                                 \
  ((RKI_DEQUE_ELEM_PTR(self))rki_deque_peek_at(RKI_DEQUE_ARGS(self), (index)))

/// @brief Returns a pointer to the first element: `T*` for `Deque(T)* self`, `const T*` for
/// `const Deque(T)* self`.
/// @return Pointer to the first element, or `NULL` if the Deque is empty
/// @note Invalidated by any later mutation of the Deque.
#define deque_peek_front(self)                                                                     \
  ((RKI_DEQUE_ELEM_PTR(self))rki_deque_peek_at(RKI_DEQUE_ARGS(self), 0))

/// @brief Returns a pointer to the last element: `T*` for `Deque(T)* self`, `const T*` for
/// `const Deque(T)* self`.
/// @return Pointer to the last element, or `NULL` if the Deque is empty
/// @note Invalidated by any later mutation of the Deque.
#define deque_peek_back(self) ((RKI_DEQUE_ELEM_PTR(self))rki_deque_peek_back(RKI_DEQUE_ARGS(self)))

/// @brief `void deque_push_front(Deque(T)* self, T value)` - Inserts `value` at the front of the
/// Deque.
/// @param value Value to insert, converted to `T` as by assignment. Evaluated once, before the
/// Deque is modified, so it may be computed from the Deque's own elements.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
#define deque_push_front(self, value) RKI_DEQUE_PUSH(rki_deque_push_front, self, value)

/// @brief `void deque_push_front_n(Deque(T)* self, const T* arr, size_t count)` - Prepends `count`
/// values from `arr` to the front of the Deque, preserving `arr`'s own order (`arr[0]` becomes the
/// new first element).
/// @param arr   Source array of `count` values; must point to the Deque's element type
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
#define deque_push_front_n(self, arr, count)                                                       \
  RKI_DEQUE_PUSH_N(rki_deque_push_front_n, self, arr, count)

/// @brief `void deque_push_back(Deque(T)* self, T value)` - Inserts `value` at the back of the
/// Deque.
/// @param value Value to insert, converted to `T` as by assignment. Evaluated once, before the
/// Deque is modified, so it may be computed from the Deque's own elements.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
#define deque_push_back(self, value) RKI_DEQUE_PUSH(rki_deque_push_back, self, value)

/// @brief `void deque_push_back_n(Deque(T)* self, const T* arr, size_t count)` - Appends `count`
/// values from `arr` to the back of the Deque, in order, as a single bulk operation.
/// @param arr   Source array of `count` values; must point to the Deque's element type
/// @param count Number of values to append
/// @note Reserves once and copies in at most two segments, instead of re-checking capacity per
/// element like `count` individual `deque_push_back()` calls would.
/// @note May reallocate the backing buffer, invalidating prior pointers into it.
/// @attention `arr[0..count)` must not overlap the Deque's own backing allocation, for the same
/// reasons documented on `deque_push_front_n()`.
#define deque_push_back_n(self, arr, count)                                                        \
  RKI_DEQUE_PUSH_N(rki_deque_push_back_n, self, arr, count)

/// @brief `T deque_pop_front(Deque(T)* self)` - Removes and returns the first element.
/// @return The (former) first element, as an rvalue
/// @pre The Deque is nonempty; use `deque_try_pop_front()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
#define deque_pop_front(self)            RKI_DEQUE_POP(rki_deque_pop_front, self)

/// @brief `T deque_pop_back(Deque(T)* self)` - Removes and returns the last element.
/// @return The (former) last element, as an rvalue
/// @pre The Deque is nonempty; use `deque_try_pop_back()` to check safely.
/// @note The precondition is asserted in debug builds; invalid access is undefined in release.
#define deque_pop_back(self)             RKI_DEQUE_POP(rki_deque_pop_back, self)

/// @brief `bool deque_try_pop_front(Deque(T)* self, T* out)` - Removes the first element and
/// writes it to `*out`, if the Deque is nonempty.
/// @param out Destination for the removed value; must point to the Deque's element type. Left
/// untouched if the Deque is empty.
/// @return `true` if an element was removed, `false` if the Deque was empty
#define deque_try_pop_front(self, out)   RKI_DEQUE_TRY_POP(rki_deque_try_pop_front, self, out)

/// @brief `bool deque_try_pop_back(Deque(T)* self, T* out)` - Removes the last element and writes
/// it to `*out`, if the Deque is nonempty.
/// @param out Destination for the removed value; must point to the Deque's element type. Left
/// untouched if the Deque is empty.
/// @return `true` if an element was removed, `false` if the Deque was empty
#define deque_try_pop_back(self, out)    RKI_DEQUE_TRY_POP(rki_deque_try_pop_back, self, out)

/// @brief Visits every element in front-to-back order.
/// @param self Pointer to the Deque. Evaluated once.
/// @param it   Iterator name. Element constness follows self.
/// @note break stops traversal; continue advances to the next element.
/// @note Do not structurally modify the Deque during traversal.
///
/// Usage:
/// ```c
/// deque_foreach(&q, it) { printf("%d\n", *it); }
/// ```
#define deque_foreach(self, it)          RKI_DEQUE_FOREACH(self, it, 0)

/// @brief Like `deque_foreach()`, but iterates in back-to-front order. Same parameters and
/// contract.
#define deque_foreach_reversed(self, it) RKI_DEQUE_FOREACH(self, it, 1)

/// @brief Erases every element satisfying `pred`, preserving the retained elements' relative
/// order.
/// @param self Pointer to a mutable Deque. Evaluated once.
/// @param it   Iterator name. Access the current element through `*it`.
/// @param pred Predicate expression, evaluated once per original element.
/// @note The predicate must not structurally modify the Deque.
/// @note Does not allocate or change capacity.
///
/// Usage:
/// ```c
/// deque_erase_if(&q, it, *it % 2 == 0); // remove even numbers
/// ```
#define deque_erase_if(self, it, pred)   RKI_DEQUE_ERASE_IF(self, it, pred)

#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

// Element-type-independent Deque state, shared by every Deque(T). The shared functions below only
// ever access it through its own declared field types; elements are reached through `data` plus a
// byte offset and copied with memcpy, so they are never accessed through a mismatched type.
typedef struct RKI_DequeBase {
  void*  data;
  size_t head, count, cap;
  RK_IFALLOC(Allocator alloc;)
} RKI_DequeBase;

// `rki_elem_type` overlays `base.data` but is an unevaluated type marker only: the macros use it in
// typeof/sizeof/alignof to recover T, and it is never read or written at runtime.
#define RKI_DEQUE_DEFINE(T)                                                                        \
  typedef struct Deque(T) {                                                                        \
    union {                                                                                        \
      RKI_DequeBase base;                                                                          \
      T*            rki_elem_type; /* type marker only */                                          \
    };                                                                                             \
  } Deque(T);

#define RKI_DEQUE_T(self) typeof(*(self)->rki_elem_type)
// Element pointer type matching the Deque's constness. Constness is detected through `count`, a
// member of a named type: matching on `const typeof(*(self))*` would spell `const const Deque`
// for a const Deque (MSVC C4114).
#define RKI_DEQUE_ITER_PTR(self)                                                                   \
  _Generic(&(self)->base.count,                                                                    \
      const size_t*: (const RKI_DEQUE_T(self)*)0,                                                  \
      default: (RKI_DEQUE_T(self)*)0)
#define RKI_DEQUE_ELEM_PTR(self)             typeof(RKI_DEQUE_ITER_PTR(self))
// The leading arguments of every shared function: `self` evaluated once, the element layout taken
// from the type marker without evaluating anything.
#define RKI_DEQUE_ARGS(self)                 &(self)->base, sizeof(RKI_DEQUE_T(self))
#define RKI_DEQUE_ARGS_A(self)               RKI_DEQUE_ARGS(self), alignof(RKI_DEQUE_T(self))
// Converts `ptr` to `T*`/`const T*` as by initialization, so a pointer to any other element type
// is rejected at compile time, and evaluates it once.
#define RKI_DEQUE_CHECK_PTR(self, ptr)       (((RKI_DEQUE_T(self)* [1]){(ptr)})[0])
#define RKI_DEQUE_CHECK_CONST_PTR(self, ptr) (((const RKI_DEQUE_T(self)* [1]){(ptr)})[0])

#define RKI_DEQUE_INIT(T, cap, alloc)                                                              \
  ((Deque(T)){.base = rki_deque_init(sizeof(T), alignof(T), (cap)RK_IFALLOC(, (alloc)))})
#define RKI_DEQUE_INIT3(T, cap, alloc) RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_DEQUE_INIT(T, cap, alloc))
#define RKI_DEQUE_INIT2(T, cap)        RKI_DEQUE_INIT(T, cap, alloc_ctx)

#define RKI_DEQUE_FROM(T, arr, n, alloc)                                                           \
  ((Deque(T)){.base = rki_deque_from(sizeof(T), alignof(T), (((const T* [1]){(arr)})[0]),          \
                                     (n)RK_IFALLOC(, (alloc)))})
#define RKI_DEQUE_FROM4(T, arr, n, alloc)                                                          \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_DEQUE_FROM(T, arr, n, alloc))
#define RKI_DEQUE_FROM3(T, arr, n)    RKI_DEQUE_FROM(T, arr, n, alloc_ctx)

#define RKI_DEQUE_RELEASE(self)       rki_deque_release(RKI_DEQUE_ARGS_A(self))
#define RKI_DEQUE_RESERVE(self, cap)  rki_deque_reserve(RKI_DEQUE_ARGS_A(self), (cap))
#define RKI_DEQUE_SHRINK_TO_FIT(self) rki_deque_shrink_to_fit(RKI_DEQUE_ARGS_A(self))
#define RKI_DEQUE_ASSIGN(self, arr, n)                                                             \
  rki_deque_assign(RKI_DEQUE_ARGS_A(self), RKI_DEQUE_CHECK_CONST_PTR(self, arr), (n))

// The value is first copied into a one-element array of T: converted as by assignment, evaluated
// before the call (and so before the Deque changes), and passed by address for a byte copy.
#define RKI_DEQUE_PUSH(fn, self, value) fn(RKI_DEQUE_ARGS_A(self), (RKI_DEQUE_T(self)[1]){(value)})
#define RKI_DEQUE_PUSH_N(fn, self, arr, n)                                                         \
  fn(RKI_DEQUE_ARGS_A(self), RKI_DEQUE_CHECK_CONST_PTR(self, arr), (n))
// The vacated slot stays valid until the next insertion; it is read immediately. The comma
// operator makes the result an rvalue in C, so a popped element cannot be assigned to.
#define RKI_DEQUE_POP(fn, self)          ((void)0, *(RKI_DEQUE_T(self)*)fn(RKI_DEQUE_ARGS(self)))
#define RKI_DEQUE_TRY_POP(fn, self, out) fn(RKI_DEQUE_ARGS(self), RKI_DEQUE_CHECK_PTR(self, out))

///////////////////////////////////////// Shared operations ////////////////////////////////////////

rklib_fun rk_pure rk_forceinline size_t rki_deque_slot_index(const RKI_DequeBase* self, size_t i) {
  return (self->head + i) & (self->cap - 1);
}
// Returns a mutable pointer even for a const Deque: `data` is a `void*` member, so no const is cast
// away here. The macros convert the result to `T*` or `const T*` according to the Deque's
// constness.
rklib_fun rk_pure rk_forceinline void* rki_deque_slot(const RKI_DequeBase* self, size_t elsize,
                                                      size_t i) {
  return (char*)self->data + rk_mult(elsize, rki_deque_slot_index(self, i));
}

rklib_fun RKI_DequeBase rki_deque_init(size_t elsize, size_t elalign,
                                       size_t cap RK_IFALLOC(, Allocator alloc)) {
  RK_IFALLOC(RKI_assert_allocator_valid(alloc);)
  RKI_DequeBase result = {rk_null, 0, 0, 0 RK_IFALLOC(, alloc)};
  if (cap) {
    cap = stdc_bit_ceil(rk_MAX((size_t)8, cap));
    rk_assert(cap && "Deque capacity overflow");
    result.data = alloc_allocate(rk_mult(elsize, cap), elalign RK_IFALLOC(, alloc));
    result.cap  = cap;
  }
  return result;
}

rklib_fun void rki_deque_release(RKI_DequeBase* self, size_t elsize, size_t elalign) {
  if (self->data) {
    alloc_deallocate(self->data, rk_mult(elsize, self->cap), elalign RK_IFALLOC(, self->alloc));
  }
  self->data = rk_null, self->head = self->count = self->cap = 0;
}

rklib_fun void rki_deque_clear(RKI_DequeBase* self) { self->head = self->count = 0; }

rklib_fun void rki_deque_realloc_to(RKI_DequeBase* self, size_t elsize, size_t elalign,
                                    size_t new_cap) {
  char* data = (char*)alloc_allocate(rk_mult(elsize, new_cap), elalign RK_IFALLOC(, self->alloc));
  if (self->count) {
    size_t first = self->count < self->cap - self->head ? self->count : self->cap - self->head;
    rk_memcpy(data, (char*)self->data + rk_mult(elsize, self->head), rk_mult(elsize, first));
    if (first < self->count) {
      rk_memcpy(data + rk_mult(elsize, first), self->data, rk_mult(elsize, self->count - first));
    }
  }
  if (self->data) {
    alloc_deallocate(self->data, rk_mult(elsize, self->cap), elalign RK_IFALLOC(, self->alloc));
  }
  self->data = data, self->head = 0, self->cap = new_cap;
}

rklib_fun void rki_deque_reserve(RKI_DequeBase* self, size_t elsize, size_t elalign,
                                 size_t requested) {
  if (requested <= self->cap) { return; }
  size_t cap = stdc_bit_ceil(rk_MAX((size_t)8, requested));
  rk_assert(cap && "Deque capacity overflow");
  RK_IFALLOC(RKI_set_alloc_fallback(self->alloc);)
  rki_deque_realloc_to(self, elsize, elalign, cap);
}

rklib_fun void rki_deque_shrink_to_fit(RKI_DequeBase* self, size_t elsize, size_t elalign) {
  if (!self->count) {
    rki_deque_release(self, elsize, elalign);
    return;
  }
  size_t cap = stdc_bit_ceil(rk_MAX((size_t)8, self->count));
  if (cap == self->cap) { return; }
  RK_IFALLOC(RKI_set_alloc_fallback(self->alloc);)
  rki_deque_realloc_to(self, elsize, elalign, cap);
}

// Not rk_pure: the asserts are side effects that must survive a discarded result (e.g.
// `(void)deque_at(&q, i)`).
rklib_fun void* rki_deque_at(const RKI_DequeBase* self, size_t elsize, size_t i) {
  rk_assert(i < self->count && "Access out of bounds of Deque.");
  return rki_deque_slot(self, elsize, i);
}
rklib_fun void* rki_deque_front(const RKI_DequeBase* self, size_t elsize) {
  rk_assert(self->count && "Cannot access front of empty deque");
  return rki_deque_slot(self, elsize, 0);
}
rklib_fun void* rki_deque_back(const RKI_DequeBase* self, size_t elsize) {
  rk_assert(self->count && "Cannot access back of empty deque");
  return rki_deque_slot(self, elsize, self->count - 1);
}
rklib_fun rk_pure void* rki_deque_peek_at(const RKI_DequeBase* self, size_t elsize, size_t i) {
  return i < self->count ? rki_deque_slot(self, elsize, i) : rk_null;
}
rklib_fun rk_pure void* rki_deque_peek_back(const RKI_DequeBase* self, size_t elsize) {
  return self->count ? rki_deque_slot(self, elsize, self->count - 1) : rk_null;
}

rklib_fun void rki_deque_grow_one(RKI_DequeBase* self, size_t elsize, size_t elalign) {
  if (self->count == self->cap) {
    rk_assert(self->cap <= SIZE_MAX / 2 && "Deque capacity overflow");
    rki_deque_reserve(self, elsize, elalign, self->cap ? self->cap * 2 : 8);
  }
}
rklib_fun void rki_deque_push_front(RKI_DequeBase* self, size_t elsize, size_t elalign,
                                    const void* value) {
  rki_deque_grow_one(self, elsize, elalign);
  self->head = (self->head - 1) & (self->cap - 1);
  ++self->count;
  rk_memcpy(rki_deque_slot(self, elsize, 0), value, elsize);
}
rklib_fun void rki_deque_push_back(RKI_DequeBase* self, size_t elsize, size_t elalign,
                                   const void* value) {
  rki_deque_grow_one(self, elsize, elalign);
  rk_memcpy(rki_deque_slot(self, elsize, self->count), value, elsize);
  ++self->count;
}

// The pops return the vacated slot, whose bytes stay intact until the next insertion.
rklib_fun void* rki_deque_pop_front(RKI_DequeBase* self, size_t elsize) {
  rk_assert(self->count && "Cannot pop an empty deque");
  void* slot = rki_deque_slot(self, elsize, 0);
  self->head = (self->head + 1) & (self->cap - 1);
  if (!--self->count) { self->head = 0; }
  return slot;
}
rklib_fun void* rki_deque_pop_back(RKI_DequeBase* self, size_t elsize) {
  rk_assert(self->count && "Cannot pop an empty deque");
  void* slot = rki_deque_slot(self, elsize, self->count - 1);
  if (!--self->count) { self->head = 0; }
  return slot;
}
rklib_fun bool rki_deque_try_pop_front(RKI_DequeBase* self, size_t elsize, void* out) {
  if (!self->count) { return false; }
  rk_memcpy(out, rki_deque_pop_front(self, elsize), elsize);
  return true;
}
rklib_fun bool rki_deque_try_pop_back(RKI_DequeBase* self, size_t elsize, void* out) {
  if (!self->count) { return false; }
  rk_memcpy(out, rki_deque_pop_back(self, elsize), elsize);
  return true;
}

// Copies n elements from arr into consecutive logical positions starting at `start`, in at most
// two segments (the ring may wrap).
rklib_fun void rki_deque_copy_in(RKI_DequeBase* self, size_t elsize, size_t start, const void* arr,
                                 size_t n) {
  size_t first = self->cap - start < n ? self->cap - start : n;
  rk_memcpy((char*)self->data + rk_mult(elsize, start), arr, rk_mult(elsize, first));
  if (first < n) {
    rk_memcpy(self->data, (const char*)arr + rk_mult(elsize, first), rk_mult(elsize, n - first));
  }
}
rklib_fun void rki_deque_push_front_n(RKI_DequeBase* self, size_t elsize, size_t elalign,
                                      const void* arr, size_t n) {
  if (!n) { return; }
  rk_assert(n <= SIZE_MAX - self->count && "Deque capacity overflow");
  rki_deque_reserve(self, elsize, elalign, self->count + n);
  self->head = (self->head - n) & (self->cap - 1);
  rki_deque_copy_in(self, elsize, self->head, arr, n);
  self->count += n;
}
rklib_fun void rki_deque_push_back_n(RKI_DequeBase* self, size_t elsize, size_t elalign,
                                     const void* arr, size_t n) {
  if (!n) { return; }
  rk_assert(n <= SIZE_MAX - self->count && "Deque capacity overflow");
  rki_deque_reserve(self, elsize, elalign, self->count + n);
  rki_deque_copy_in(self, elsize, rki_deque_slot_index(self, self->count), arr, n);
  self->count += n;
}
rklib_fun void rki_deque_assign(RKI_DequeBase* self, size_t elsize, size_t elalign, const void* arr,
                                size_t n) {
  rki_deque_clear(self);
  rki_deque_push_back_n(self, elsize, elalign, arr, n);
}
rklib_fun RKI_DequeBase rki_deque_from(size_t elsize, size_t elalign, const void* arr,
                                       size_t n RK_IFALLOC(, Allocator alloc)) {
  RKI_DequeBase d = rki_deque_init(elsize, elalign, n RK_IFALLOC(, alloc));
  rki_deque_push_back_n(&d, elsize, elalign, arr, n);
  return d;
}

///////////////////////////////////////// Iteration ////////////////////////////////////////////////

// `reversed` is a constant 0 or 1, folded away by the compiler.
#define RKI_DEQUE_FOREACH(self, it, reversed)                                                      \
  for (struct {                                                                                    \
         typeof(*(self))* deque;                                                                   \
         size_t           idx, count;                                                              \
       } rki_var_state = {(self), 0, 0};                                                           \
       rki_var_state.deque && (rki_var_state.count = rki_var_state.deque->base.count, 1);          \
       rki_var_state.deque = rk_null)                                                              \
    for (RKI_DEQUE_ELEM_PTR(rki_var_state.deque) it = rk_null;                                     \
         rki_var_state.idx < rki_var_state.count                                                   \
         && (it = (RKI_DEQUE_ELEM_PTR(rki_var_state.deque))rki_deque_slot(                         \
                 &rki_var_state.deque->base, sizeof(*it),                                          \
                 (reversed) ? rki_var_state.count - 1 - rki_var_state.idx : rki_var_state.idx),    \
            1);                                                                                    \
         ++rki_var_state.idx)

#define RKI_DEQUE_ERASE_IF(self, it, pred)                                                         \
  do {                                                                                             \
    typeof(*(self))* const rki_var_deque = (self);                                                 \
    if (!rki_var_deque || !rki_var_deque->base.count) { break; }                                   \
    RKI_DequeBase* const rki_var_base  = &rki_var_deque->base;                                     \
    const size_t         rki_var_count = rki_var_base->count;                                      \
    const size_t         rki_var_size  = sizeof(RKI_DEQUE_T(rki_var_deque));                       \
    size_t               rki_var_write = 0;                                                        \
    for (size_t rki_var_idx = 0; rki_var_idx < rki_var_count; ++rki_var_idx) {                     \
      RKI_DEQUE_T(rki_var_deque)* const it                                                         \
          = (RKI_DEQUE_T(rki_var_deque)*)rki_deque_slot(rki_var_base, rki_var_size, rki_var_idx);  \
      if (!(pred)) {                                                                               \
        if (rki_var_write != rki_var_idx) {                                                        \
          *(RKI_DEQUE_T(rki_var_deque)*)rki_deque_slot(rki_var_base, rki_var_size, rki_var_write)  \
              = *it;                                                                               \
        }                                                                                          \
        ++rki_var_write;                                                                           \
      }                                                                                            \
    }                                                                                              \
    rki_var_base->count = rki_var_write;                                                           \
    if (!rki_var_write) { rki_var_base->head = 0; }                                                \
  } while (0)

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
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
