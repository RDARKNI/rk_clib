// SPDX-License-Identifier: MIT
/// @file rk_pool.h
/// @version 1.0.0
/// @defgroup rk_pool Pool Allocator Interface
/// @brief Type-safe generic pool allocator for C
///
/// This header provides a type-safe, generic pool allocator system for C. It supports:
/// - **Dynamic pools** (runtime capacity) using `Pool(T)`
/// - **Static pools** (compile-time fixed capacity) using `Pool(T, C)`
/// - Allocation and deallocation tracking using `bitset`
/// - Type-safe macros for creating, accessing, and releasing pool elements
/// - Iteration over active elements via `pool_foreach`
///
/// The system is heavily macro-based to simulate function overloading and type safety in plain C.
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{

#ifndef RK_POOL_H
#define RK_POOL_H
#include "rk_alloc.h"
#include "rk_bitset.h"
RKI_HEADER_BEGIN

/// @brief Defines a pool type and associated functions for type `T` Depending on the arguments,
/// this macro defines either a dynamic pool (`Pool(T)`) or a static/fixed pool (`Pool(T, C)`).
/// @param T Type of elements stored in the pool
/// @param C Capacity of the pool if static
#define POOL_DEFINE(T, ...)           RKI_STATOVERLOAD(RKI_POOL_DEFINE, T, ##__VA_ARGS__)

/// @brief Alias for the pool type (dynamic or static).
/// @param T Type of elements stored in the pool
/// @param C Capacity of the pool if static
/// @note Static Pools take a second capacity parameter
#define Pool(T, ...)                  RKI_STATOVERLOAD__(RKI_POOL, T, ##__VA_ARGS__)
#define StaticPool(T, CAP)            Pool_##CAP##_##T
#define DynPool(T)                    Pool_##T

/// @brief `Pool(T) pool_init(T, size_t cap, Allocator alloc = alloc_ctx)` - Initializes a dynamic
/// pool with given capacity.
/// @param T Element type
/// @param cap Desired capacity
/// @param alloc Optional allocator
/// @return Initialized pool struct
#define pool_init(T, _cap, ...)       RKI_OVERLOAD(RKI_DPOOL_INIT, T, _cap, ##__VA_ARGS__)

/// @brief Compile-time zero-initializer for a `Pool(T, C)` (`StaticPool`). Suitable for global and
/// static variables. No memory is allocated.
/// @note Named after `StaticPool`, the type it initializes — not to be confused with the (removed)
/// `_init_static` convention other containers used for static-storage-duration-safe initializers.
#define staticpool_init               {RKI_ZINIT}

/// @brief `void pool_release(Pool(T, ...)* self)` - Releases the associated resources of the pool
/// (if the pool is dynamic) and resets its members. For static pools, this resets the allocation
/// bitset but does not modify the underlying element storage.
#define pool_release(self)            ((void)RKI_POOL_RELEASE(self))

/// @brief `size_t pool_cap(Pool(T, ...)* self)` - Returns the total capacity of the pool.
#define pool_cap(self)                ((size_t)RKI_POOL_CAP(self))

/// @brief `Allocator pool_allocator(Pool(T)* self)` - Returns the Allocator the (dynamic) pool was
/// constructed with, or `alloc_ctx` if the pool was never initialized or custom allocators are
/// disabled.
#define pool_allocator(self)          RKI_allocatorof(self)

/// @brief `size_t pool_used(Pool(T)* self)` - Returns the number of active (allocated) elements in
/// the pool.
#define pool_used(self)               ((size_t)RKI_POOL_USED(self))
#define pool_count(self)              pool_used(self) /// @brief Alias for `pool_used`

/// @brief `size_t pool_remaining(Pool(T)* self)` - Returns the number of free slots remaining in
/// the pool.
#define pool_remaining(self)          ((size_t)RKI_POOL_REMAINING(self))

/// @brief Returns `true` iff the pool is empty.
#define pool_is_empty(self)           ((bool)(pool_used(self) == 0))

/// @brief Returns `true` iff the pool is full.
#define pool_is_full(self)            ((bool)(pool_remaining(self) == 0))

/// @brief `Pool(T)* pool_clear(Pool(T)* self)` - Marks all elements in the pool as reusable.
/// @return `self`, for chaining
#define pool_clear(self)              ((typeof(self))RKI_POOL_CLEAR(self))

/// @brief `T* pool_new(Pool(T)* self)` - Allocates a new element in the pool.
/// @return Pointer to the newly allocated element
#define pool_new(self)                ((RKI_POOL_T(self)*)RKI_POOL_NEW(self))

/// @brief `T* pool_try_new(Pool(T)* self)` - Like `pool_new()`, but returns `NULL` if full instead
/// of running `RK_POOL_FAIL()`.
#define pool_try_new(self)            ((RKI_POOL_T(self)*)RKI_POOL_TRY_NEW(self))

/// @brief `T* pool_put(Pool(T)* self, T el)` - Allocates a new element and stores a copy of the
/// value.
/// @return Pointer to the inserted element
#define pool_put(self, el)            ((RKI_POOL_T(self)*)RKI_POOL_PUT(self, el))

/// @brief `T* pool_try_put(Pool(T)* self, T el)` - Like `pool_put()`, but returns `NULL` if full
/// instead of running `RK_POOL_FAIL()`.
#define pool_try_put(self, el)        ((RKI_POOL_T(self)*)RKI_POOL_TRY_PUT(self, el))

/// @brief `void pool_delete(Pool(T)* self, T* ptr)` - Frees an element in the pool.
#define pool_delete(self, ptr)        ((void)RKI_POOL_DELETE(self, ptr))

/// @brief Visits every allocated element in the pool.
/// @param self Pointer to the Pool. Evaluated once.
/// @param it   Iterator name (a pointer to an element; access via `*it`).
/// @note Elements are const when accessed through a pointer to a const Pool.
/// @note break stops traversal; continue advances to the next allocated element.
/// @note There is deliberately no `pool_foreach_reversed`: unlike Vec/Deque/Str (positional
/// sequences) or the trees (sorted by key), a Pool's iteration order is just the ascending bitset
/// index of whichever slots the first-fit allocator happened to occupy -- an implementation
/// artifact, not a property of the data a caller can rely on (the same reason Dict/Set's
/// "unspecified slot order" never got one either).
///
/// Usage:
/// ```c
/// pool_foreach(&my_pool, elem) {
///     printf("%d\n", *elem);
/// }
/// ```
#define pool_foreach(self, it)        RKI_POOL_FOREACH(self, it)

/// @brief Erases every allocated element satisfying `pred`.
/// @param self Pointer to a mutable Pool. Evaluated once.
/// @param it   Iterator name (access via `*it`).
/// @param pred Predicate expression, evaluated once per original allocated element.
/// @note The predicate must not structurally modify the Pool.
///
/// Usage:
/// ```c
/// pool_erase_if(&my_pool, it, *it % 2 == 0); // delete even values
/// ```
#define pool_erase_if(self, it, pred) RKI_POOL_ERASE_IF(self, it, pred)

#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RKI_POOL2 StaticPool
#define RKI_POOL_DEFINE2(T, C)                                                                     \
  typedef struct StaticPool(T, C) {                                                                \
    bitset(C) data;                                                                                \
    union {                                                                                        \
      T els[C], rki_els_dummy[C];                 /* only for _Generic, never read */              \
      union { unsigned char cap, els[1]; } _pool; /* only for _Generic, never read */              \
    };                                                                                             \
  } StaticPool(T, C)

/// @brief to allow for type-generic access of dynamic pools todo c++ UB union
typedef struct RKI_DynPool {
  RK_IFALLOC(Allocator alloc;)
  bitset data;
  size_t cap;
  union {
    void* els;
    char  rki_els_dummy[1];
  };
} RKI_DynPool;

#define RKI_POOL_DEFINE1(T)                                                                        \
  typedef struct DynPool(T) {                                                                      \
    union {                                                                                        \
      RKI_DynPool _pool;                                                                           \
      struct {                                                                                     \
        RK_IFALLOC(Allocator alloc;)                                                               \
        bitset data;                                                                               \
        size_t cap;                                                                                \
        union {                                                                                    \
          T*   els;              /*type marker, never accessed directly during runtime */          \
          char rki_els_dummy[1]; /*unselected _Generic branch countof */                           \
        };                                                                                         \
      };                                                                                           \
    };                                                                                             \
  } DynPool(T)

#define RKI_POOL1        DynPool

#define RKI_POOL_T(self) typeof(*(self)->els)

#define RKI_POOL_DISPATCH(self, if_stat, if_dyn)                                                   \
  rk_static_if(sizeof((self)->_pool) == 1, if_stat, if_dyn)

rklib_fun rk_forceinline void* rki_dpool_init(size_t elsize, size_t elalign, RKI_DynPool* self,
                                              size_t cap RK_IFALLOC(, Allocator alloc)) {
  RKI_assert_allocator_valid(alloc);
  self->cap = cap;
  RK_IFALLOC(self->alloc = alloc;)
  self->data
      = bitset_clear_all(alloc_new(bitset_word, bitset_words(cap) RK_IFALLOC(, self->alloc)), cap);
  self->els = alloc_allocate(rk_mult(elsize, cap), elalign RK_IFALLOC(, alloc));
  return self;
}
#define RKI_DPOOL_INIT(T, _cap, _alloc)                                                            \
  (*((Pool(T)*)rki_dpool_init(sizeof(T), alignof(T), (RKI_DynPool*)((Pool(T)[1]){RKI_ZINIT}),      \
                              _cap RK_IFALLOC(, _alloc))))
#define RKI_DPOOL_INIT3(T, _cap, _alloc)                                                           \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_DPOOL_INIT(T, _cap, _alloc))
#define RKI_DPOOL_INIT2(T, _cap) RKI_DPOOL_INIT(T, _cap, alloc_ctx)

rklib_fun rk_forceinline void rki_dpool_release(size_t elsize, size_t elalign, RKI_DynPool* self) {
  if (!self->cap) { return; }
  alloc_deallocate(self->els, rk_mult(elsize, self->cap), elalign RK_IFALLOC(, self->alloc));
  alloc_delete(self->data, bitset_words(self->cap) RK_IFALLOC(, self->alloc));
  self->cap = 0, self->data = rk_null, self->els = rk_null;
}

#define RKI_POOL_RELEASE(self)                                                                     \
  RKI_POOL_DISPATCH(self, (void)bitset_clear_all((self)->data, RKI_SPOOL_CAP(self)),               \
                    rki_dpool_release(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                   \
                                      (RKI_DynPool*)&((self)->_pool)))

#define RKI_SPOOL_CAP(self) rk_COUNTOF((self)->rki_els_dummy)
#define RKI_POOL_CAP(self)  RKI_POOL_DISPATCH(self, RKI_SPOOL_CAP(self), (size_t)(self)->_pool.cap)

rklib_fun rk_pure rk_forceinline size_t rki_dpool_used(const RKI_DynPool* self) {
  return bitset_count_ones(self->data, self->cap);
}
#define RKI_POOL_USED(self)                                                                        \
  RKI_POOL_DISPATCH(self, bitset_count_ones((self)->data, RKI_SPOOL_CAP(self)),                    \
                    rki_dpool_used((RKI_DynPool*)&((self)->_pool)))

rklib_fun rk_pure rk_forceinline size_t rki_dpool_remaining(const RKI_DynPool* self) {
  return bitset_count_zeros(self->data, self->cap);
}
#define RKI_POOL_REMAINING(self)                                                                   \
  RKI_POOL_DISPATCH(self, bitset_count_zeros((self)->data, RKI_SPOOL_CAP(self)),                   \
                    rki_dpool_remaining((RKI_DynPool*)&((self)->_pool)))

rklib_fun rk_forceinline void* rki_dpool_clear(RKI_DynPool* self) {
  bitset_clear_all(self->data, self->cap);
  return self;
}
#define RKI_POOL_CLEAR(self)                                                                       \
  RKI_POOL_DISPATCH(self, bitset_clear_all((self)->data, RKI_SPOOL_CAP(self)),                     \
                    rki_dpool_clear((RKI_DynPool*)&((self)->_pool)))

rklib_fun rk_forceinline void rki_spool_delete(size_t elsize, size_t cap, size_t offset, void* self,
                                               void* ptr) {
  bitset_clear((bitset)self, cap, (size_t)((size_t)((char*)ptr - ((char*)self + offset)) / elsize));
}
rklib_fun rk_forceinline void rki_dpool_delete(size_t elsize, RKI_DynPool* self, void* ptr) {
  bitset_clear(self->data, self->cap, (size_t)((size_t)((char*)ptr - (char*)self->els) / elsize));
}
#define RKI_POOL_DELETE(self, ptr)                                                                 \
  RKI_POOL_DISPATCH(self,                                                                          \
                    rki_spool_delete(RKI_POOL_SIZE(self), RKI_SPOOL_CAP(self),                     \
                                     offsetof(typeof(*(self)), els), self, ptr),                   \
                    rki_dpool_delete(RKI_POOL_SIZE(self), (RKI_DynPool*)&((self)->_pool), ptr))

rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_spool_try_new(size_t         elsize,
                                                                          size_t elalign rk_unused,
                                                                          size_t cap, size_t offset,
                                                                          void* self) {
  size_t free_slot = bitset_first_trailing_zero((bitset)self, cap);
  if (!free_slot) { return rk_null; }
  bitset_set((bitset)self, cap, free_slot - 1);
  return (char*)self + offset + elsize * (free_slot - 1);
}

rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_dpool_try_new(size_t         elsize,
                                                                          size_t elalign rk_unused,
                                                                          RKI_DynPool*   self) {
  if (!self->cap) { return rk_null; }
  size_t free_slot = bitset_first_trailing_zero(self->data, self->cap);
  if (!free_slot) { return rk_null; }
  bitset_set(self->data, self->cap, free_slot - 1);
  return (char*)self->els + elsize * (free_slot - 1);
}

#define RKI_POOL_TRY_NEW(self)                                                                     \
  RKI_POOL_DISPATCH(self,                                                                          \
                    rki_spool_try_new(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                   \
                                      RKI_SPOOL_CAP(self), offsetof(typeof(*(self)), els), self),  \
                    rki_dpool_try_new(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                   \
                                      (RKI_DynPool*)&((self)->_pool)))

rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_spool_new(size_t elsize, size_t elalign,
                                                                      size_t cap, size_t offset,
                                                                      void* self) {
  void* res = rki_spool_try_new(elsize, elalign, cap, offset, self);
  RK_POOL_FAIL(res, self, rk_null, elalign, elsize);
  return res;
}
rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_dpool_new(size_t elsize, size_t elalign,
                                                                      RKI_DynPool* self) {
  void* res = rki_dpool_try_new(elsize, elalign, self);
  RK_POOL_FAIL(res, self, rk_null, elalign, elsize);
  return res;
}

#define RKI_POOL_NEW(self)                                                                         \
  RKI_POOL_DISPATCH(                                                                               \
      self,                                                                                        \
      rki_spool_new(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self), RKI_SPOOL_CAP(self),                \
                    offsetof(typeof(*(self)), els), self),                                         \
      rki_dpool_new(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self), (RKI_DynPool*)&((self)->_pool)))

rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_spool_try_put(size_t         elsize,
                                                                          size_t elalign rk_unused,
                                                                          size_t cap, size_t offset,
                                                                          void* restrict self,
                                                                          void* restrict obj) {
  bitset data      = (bitset)self;
  size_t free_slot = bitset_first_trailing_zero(data, cap);
  if (!free_slot) { return rk_null; }
  bitset_set(data, cap, free_slot - 1);
  void* ptr = (char*)self + offset + elsize * (free_slot - 1);
  rk_memcpy(ptr, obj, elsize);
  return ptr;
}
rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_spool_put(size_t elsize, size_t elalign,
                                                                      size_t cap, size_t offset,
                                                                      void* restrict self,
                                                                      void* restrict obj) {
  void* res = rki_spool_try_put(elsize, elalign, cap, offset, self, obj);
  RK_POOL_FAIL(res, self, rk_null, elalign, elsize);
  return res;
}
rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_dpool_try_put(
    size_t elsize, size_t elalign rk_unused, RKI_DynPool* restrict self, void* restrict obj) {
  if (!self->cap) { return rk_null; }
  size_t free_slot = bitset_first_trailing_zero(self->data, self->cap);
  if (!free_slot) { return rk_null; }
  bitset_set(self->data, self->cap, free_slot - 1);
  return rk_memcpy((char*)self->els + elsize * (free_slot - 1), obj, elsize);
}
rklib_fun rk_forceinline rk_alloc_alignsize(2, 1) void* rki_dpool_put(size_t elsize, size_t elalign,
                                                                      RKI_DynPool* restrict self,
                                                                      void* restrict obj) {
  void* res = rki_dpool_try_put(elsize, elalign, self, obj);
  RK_POOL_FAIL(res, self, rk_null, elalign, elsize);
  return res;
}
#define RKI_POOL_TRY_PUT(self, el)                                                                 \
  RKI_POOL_DISPATCH(self,                                                                          \
                    rki_spool_try_put(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                   \
                                      RKI_SPOOL_CAP(self), offsetof(typeof(*(self)), els), self,   \
                                      (RKI_POOL_T(self)[1]){el}),                                  \
                    rki_dpool_try_put(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                   \
                                      (RKI_DynPool*)&((self)->_pool), (RKI_POOL_T(self)[1]){el}))

#define RKI_POOL_PUT(self, el)                                                                     \
  RKI_POOL_DISPATCH(self,                                                                          \
                    rki_spool_put(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self), RKI_SPOOL_CAP(self),  \
                                  offsetof(typeof(*(self)), els), self,                            \
                                  (RKI_POOL_T(self)[1]){el}),                                      \
                    rki_dpool_put(RKI_POOL_SIZE(self), RKI_POOL_ALIGN(self),                       \
                                  (RKI_DynPool*)&((self)->_pool), (RKI_POOL_T(self)[1]){el}))

#define RKI_POOL_SIZE(self)  sizeof(*(self)->els)
#define RKI_POOL_ALIGN(self) alignof(RKI_POOL_T(self))
// Constness is detected through `_pool.cap`, a member of a named type (unsigned char for static
// pools, size_t for dynamic ones): matching on `const typeof(*(self))*` would spell
// `const const Pool` for a const Pool (MSVC C4114), as for the Dict/Deque/tree helpers.
#define RKI_POOL_ITER_PTR(self)                                                                    \
  RKI_IGNWARN_MSC(4114, _Generic(&(self)->_pool.cap,                                               \
                      const unsigned char*: (const RKI_POOL_T(self)*)0,                            \
                      const size_t*: (const RKI_POOL_T(self)*)0,                                   \
                      default: (RKI_POOL_T(self)*)0))
/// to prevent inactive union member access in c++
#define RKI_POOL_ELS(self)                                                                         \
  RKI_POOL_DISPATCH(self, (self)->els, (typeof(RKI_POOL_ITER_PTR(self)))(self)->_pool.els)

#define RKI_STATOVERLOAD__(m, ...) rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)
#define RKI_STATOVERLOAD(m, ...)   rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)

// A single real loop: `it` is the loop variable itself, and each re-check of the condition
// (whether reached normally or via `continue`) advances to the next set bit. `break` therefore
// exits this loop directly, same as it would for a plain array loop -- unlike the old
// three-nested-loop version this replaced, where the innermost loop existed only to declare `it`
// once per index and always terminated after a single pass regardless of the body, so `break` only
// ever exited that already-terminating inner loop and silently behaved like `continue`.
#define RKI_POOL_FOREACH(self, it)                                                                 \
  for (struct {                                                                                    \
         typeof(*(self))* pool;                                                                    \
         RKI_BitsetIter   bits;                                                                    \
         size_t           idx;                                                                     \
       } rki_var_state = {(self), {rk_null, 0, 0, 0}, 0};                                          \
       rki_var_state.pool                                                                          \
       && (rki_var_state.bits.bits = rki_var_state.pool->data,                                     \
          rki_var_state.bits.count = pool_cap(rki_var_state.pool), 1);                             \
       rki_var_state.pool = rk_null)                                                               \
    for (typeof(RKI_POOL_ITER_PTR(rki_var_state.pool)) it = rk_null;                               \
         (rki_var_state.idx = rki_bitset_iter_next(&rki_var_state.bits)) != BITSET_NPOS            \
         && (it = RKI_POOL_ELS(rki_var_state.pool) + rki_var_state.idx, (void)it, 1);)

#define RKI_POOL_ERASE_IF(self, it, pred)                                                          \
  do {                                                                                             \
    typeof(self) const rki_var_pool = (self);                                                      \
    RKI_POOL_FOREACH(rki_var_pool, rki_var_cursor) {                                               \
      RKI_POOL_T(rki_var_pool)* const it = rki_var_cursor;                                         \
      (void)it;                                                                                    \
      if (pred) { pool_delete(rki_var_pool, it); }                                                 \
    }                                                                                              \
  } while (0)

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
/// @}
#endif // RK_POOL_H

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
