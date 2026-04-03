/// @file rk_pool.h
/// @version 2.0
/// @defgroup rk_pool Pool Allocator Interface
/// @brief Type-safe generic pool allocator for C
///
/// This header provides a type-safe, generic pool allocator system for C.
/// It supports:
/// - **Dynamic pools** (runtime capacity) using `Pool(T)`
/// - **Static pools** (compile-time fixed capacity) using `Pool(T, C)`
/// - Allocation and deallocation tracking using `bitset`
/// - Type-safe macros for creating, accessing, and releasing pool elements
/// - Iteration over active elements via `pool_foreach`
///
/// The system is heavily macro-based to simulate function overloading and
/// type safety in plain C.
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{

#ifndef RK_POOL_H
#define RK_POOL_H
#include "rk_alloc.h"
#include "rk_bitset.h"
RK_HEADER_BEGIN

///  @brief Defines a pool type and associated functions for type `T`
///  Depending on the arguments, this macro defines either a dynamic pool
///  (`Pool(T)`) or a static/fixed pool (`Pool(T, C)`).
///  @param T Type of elements stored in the pool
///  @param C Capacity of the pool if static
#define POOL_DEFINE(T, ...) RK__STATOVERLOAD(RK__POOL_DEFINE, T, ##__VA_ARGS__)

/// @def Pool(T, ...)
/// @brief Alias for the pool type (dynamic or static)
/// @param T Type of elements
/// @param C Capacity of the pool if static
/// @note Static Pools take a second capacity parameter
#define Pool(T, ...)        RK__STATOVERLOAD__(RK__POOL, T, ##__VA_ARGS__)
#define StaticPool(T, CAP)  pool_static_##T##_##CAP
#define DynPool(T)          pool_dynamic_##T

/// @def Pool(T)* pool_init_dynamic(T, size_t cap, Allocator alloc = alloc_ctx)
/// @brief Initializes a dynamic pool with given capacity
/// @param T Element type
/// @param cap Desired capacity
/// @param alloc Optional allocator
/// @return Initialized pool struct
#define pool_init_dynamic(T, _cap, ...)                                        \
  rk_overload(RK__Dpool_init, T, _cap, ##__VA_ARGS__)

/// @brief Static/compile-time zero-initializer for a `Pool(T, C)`. Suitable
/// for global and static variables. No memory is allocated.
#define pool_init_static       {RK_ZINIT}

/// @def `void pool_release(Pool(T, ...)* self)`
/// @brief Releases the associated resources of a pool (if the pool is dynamic)
/// and resets its members. For static pools, this simply resets the bitset and
/// does not free any memory.
/// @param self Pointer to the pool to release
#define pool_release(self)     ((void)RK__pool_release(self))

/// @def `size_t pool_cap(Pool(T, ...)* self)`
/// @brief Returns the total capacity of a pool.
#define pool_cap(self)         ((size_t)RK__pool_cap(self))

/// @def `size_t pool_used(Pool(T)* self)`
/// @brief Returns number of active (allocated) elements
/// @param self Pool pointer
/// @return Number of currently allocated elements
#define pool_used(self)        ((size_t)RK__pool_used(self))

/// @def `size_t pool_remaining(Pool(T)* self)`
/// @brief Returns number of free slots remaining in the pool
/// @param self Pool pointer
/// @return Number of remaining free elements
#define pool_remaining(self)   ((size_t)RK__pool_remaining(self))

/// @brief Returns `true` iff the pool is empty
#define pool_is_empty(self)    ((bool)(pool_used(self) == 0))

/// @brief Returns `true` iff the pool is full
#define pool_is_full(self)     ((bool)(pool_remaining(self) == 0))

/// @def `Pool(T)* pool_clear(Pool(T)* self)`
/// @brief Clears all elements in a pool
/// @param self Pool pointer
/// @return `self`, for chaining
#define pool_clear(self)       ((typeof(self))RK__pool_clear(self))

/// @def `T* pool_new(Pool(T)* self)`
/// @brief Allocates a new element in a pool
/// @param self Pool pointer
/// @return Pointer to newly allocated element
#define pool_new(self)         ((RK__poolT(self)*)RK__pool_new(self))

/// @def `T* pool_try_new(Pool(T)* self)`
/// @brief Like `pool_new()`, but returns NULL if full instead of
/// running `RK_POOL_FAIL()`.
#define pool_try_new(self)     ((RK__poolT(self)*)RK__pool_try_new(self))

/// @def `T* pool_put(Pool(T)* self, T el)`
/// @brief Allocates a new element and stores a copy of the value
/// @param self Pool pointer
/// @param el Element to insert
/// @return Pointer to the inserted element
#define pool_put(self, el)     ((RK__poolT(self)*)RK__pool_put(self, el))

/// @def `T* pool_try_put(Pool(T)* self)`
/// @brief Like `pool_put()`, but returns NULL if full instead of running
/// `RK_POOL_FAIL()`.
#define pool_try_put(self, el) ((RK__poolT(self)*)RK__pool_try_put(self, el))

/// @def `void pool_delete(Pool(T)* self, T* ptr)`
/// @brief Frees an element in the pool
/// @param self Pool pointer
/// @param ptr Pointer to the element to free
#define pool_delete(self, ptr) ((void)RK__pool_delete(self, ptr))

/// @def `pool_foreach(Pool(T)* self, it)`
/// @brief Iterates over all allocated elements in the pool.
/// Example:
/// ```c
/// pool_foreach(&my_pool, elem) {
///     printf("%d\n", *elem);
/// }
/// ```
#define pool_foreach(self, it) RK__pool_foreach(self, it)

////////////////////////////////////////////////////////////////////////////////
/////////////////////////Implementation Details/////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RK__POOL2              StaticPool
#define RK__POOL_DEFINE2(T, C)                                                 \
  typedef struct StaticPool(T, C) {                                            \
    bitset(C) data;                                                            \
    union {                                                                    \
      T    els[C];                                                             \
      T    RK__POOL_ELS[C]; /* only for _Generic, never read */                \
      char RK__POOL_CAP;    /* only for _Generic, never read */                \
      char RK__pooldummy;   /* only for _Generic, never read */                \
    };                                                                         \
  } StaticPool(T, C);

/// @brief to allow for type-generic access of dynamic pools todo c++ UB union
typedef struct RK__pool_dynamic {
  bitset data;
  size_t cap;
  void*  els;
  RK_IFALLOC(Allocator alloc;)
} RK__pool_dynamic;
#define RK__POOL1 DynPool
#define RK__POOL_DEFINE1(T)                                                    \
  typedef struct DynPool(T) {                                                  \
    union {                                                                    \
      struct {                                                                 \
        bitset data;                                                           \
        union {                                                                \
          size_t cap;                                                          \
          size_t RK__POOL_CAP;    /* only for _Generic, never read */          \
          char   RK__POOL_ELS[1]; /* only for _Generic, never read */          \
        };                                                                     \
        T* els;                                                                \
        RK_IFALLOC(Allocator alloc;)                                           \
      };                                                                       \
      RK__pool_dynamic RK__pooldummy;                                          \
    };                                                                         \
  } DynPool(T);

#ifndef __cplusplus
# define RK__poolT(self) typeof((self)->els[0])
#else
# define RK__poolT(self) std::remove_reference<decltype((self)->els[0])>::type
#endif

#define RK__pool_dispatch(self, if_stat, if_dyn)                               \
  rk_static_if(sizeof((self)->RK__POOL_CAP) == 1, if_stat, if_dyn)

static_fun rk_forceinline void*
    RK__Dpool_init(size_t elsize, size_t elalign, RK__pool_dynamic* self,
                   size_t cap RK_IFALLOC(, Allocator alloc)) {
  self->cap = cap;
  rk_assert_allocator_valid(alloc);
  RK_IFALLOC(self->alloc = alloc;)
  self->data = bitset_clear_all(
      alloc_new(bitset_word, bitset_words(cap), self->alloc), cap);
  self->els = alloc_allocate(elsize * cap, elalign, alloc);
  return self;
}
#define RK__Dpool_init3(T, _cap, _alloc)                                       \
  (*((Pool(T)*)RK__Dpool_init(sizeof(T), alignof(T),                           \
                              (RK__pool_dynamic*)((Pool(T)[1]){RK_ZINIT}),     \
                              _cap RK_IFALLOC(, _alloc))))
#define RK__Dpool_init2(T, _cap) RK__Dpool_init3(T, _cap, alloc_ctx)

#define RK__pool_cap(self)                                                     \
  RK__pool_dispatch(self, rk_COUNTOF((self)->RK__POOL_ELS),                    \
                    (size_t)(self)->RK__POOL_CAP)

static_fun rk_forceinline size_t RK__Dpool_used(const RK__pool_dynamic* self) {
  return bitset_count_ones(*(bitset*)self, self->cap);
}
#define RK__pool_used(self)                                                    \
  RK__pool_dispatch(                                                           \
      self, bitset_count_ones((bitset)self, rk_COUNTOF((self)->RK__POOL_ELS)), \
      RK__Dpool_used((RK__pool_dynamic*)&((self)->RK__pooldummy)))

static_fun rk_forceinline size_t
    RK__Dpool_remaining(const RK__pool_dynamic* self) {
  return bitset_count_zeros(*(bitset*)self, self->cap);
}
#define RK__pool_remaining(self)                                               \
  RK__pool_dispatch(                                                           \
      self,                                                                    \
      bitset_count_zeros((self)->data, rk_COUNTOF((self)->RK__POOL_ELS)),      \
      RK__Dpool_remaining((RK__pool_dynamic*)&((self)->RK__pooldummy)))

static_fun rk_forceinline void* RK__Dpool_clear(RK__pool_dynamic* self) {
  bitset_clear_all(self->data, self->cap);
  return self;
}
#define RK__pool_clear(self)                                                   \
  RK__pool_dispatch(                                                           \
      self, bitset_clear_all((bitset)self, rk_COUNTOF((self)->RK__POOL_ELS)),  \
      RK__Dpool_clear((RK__pool_dynamic*)&((self)->RK__pooldummy)))

static_fun rk_forceinline void RK__Spool_delete(size_t elsize, size_t cap,
                                                void* self, void* ptr) {
  bitset_clear((bitset)self, cap,
               ((char*)ptr - ((char*)self + bitset_bytes(cap))) / elsize);
}
static_fun rk_forceinline void
    RK__Dpool_delete(size_t elsize, RK__pool_dynamic* self, void* ptr) {
  bitset_clear(self->data, self->cap, ((char*)ptr - (char*)self->els) / elsize);
}

#define RK__pool_delete(self, ptr)                                             \
  RK__pool_dispatch(                                                           \
      self,                                                                    \
      RK__Spool_delete(sizeof((self)->els[0]),                                 \
                       rk_COUNTOF((self)->RK__POOL_ELS), self, ptr),           \
      RK__Dpool_delete(sizeof((self)->els[0]),                                 \
                       (RK__pool_dynamic*)&((self)->RK__pooldummy), ptr))

static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Spool_try_new(
    size_t elsize, size_t align, size_t cap, void* self) {
  size_t free_slot = bitset_first_trailing_zero((bitset)self, cap);
  if (!free_slot) { return rk_null; }
  RK_POOL_FAIL(free_slot, self, rk_null, align, elsize);
  bitset_set((bitset)self, cap, free_slot - 1);
  return (char*)self + bitset_bytes(cap) + elsize * (free_slot - 1);
}

static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Spool_new(
    size_t elsize, size_t align, size_t cap, void* self) {
  void* res = RK__Spool_try_new(elsize, align, cap, self);
  RK_POOL_FAIL(res, self, rk_null, align, elsize);
  return res;
}
static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Dpool_try_new(
    size_t elsize, size_t align rk_unused, RK__pool_dynamic* self) {
  if (!self->cap) { return rk_null; }
  size_t free_slot = bitset_first_trailing_zero(self->data, self->cap);
  if (!free_slot) { return rk_null; }
  bitset_set(self->data, self->cap, free_slot - 1);
  return (char*)self->els + elsize * (free_slot - 1);
}
static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Dpool_new(
    size_t elsize, size_t align, RK__pool_dynamic* self) {
  void* res = RK__Dpool_try_new(elsize, align, self);
  RK_POOL_FAIL(res, self, rk_null, align, elsize);
  return res;
}

#define RK__pool_try_new(self)                                                 \
  RK__pool_dispatch(                                                           \
      self,                                                                    \
      RK__Spool_try_new(sizeof((self)->els[0]), alignof(RK__poolT(self)),      \
                        rk_COUNTOF((self)->RK__POOL_ELS), self),               \
      RK__Dpool_try_new(sizeof((self)->els[0]), alignof(RK__poolT(self)),      \
                        (RK__pool_dynamic*)&((self)->RK__pooldummy)))

#define RK__pool_new(self)                                                     \
  RK__pool_dispatch(                                                           \
      self,                                                                    \
      RK__Spool_new(sizeof((self)->els[0]), alignof(RK__poolT(self)),          \
                    rk_COUNTOF((self)->RK__POOL_ELS), self),                   \
      RK__Dpool_new(sizeof((self)->els[0]), alignof(RK__poolT(self)),          \
                    (RK__pool_dynamic*)&((self)->RK__pooldummy)))

static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Spool_try_put(
    size_t elsize, size_t align rk_unused, size_t cap, void* restrict self,
    void* restrict obj) {
  bitset data      = (bitset)self;
  size_t free_slot = bitset_first_trailing_zero(data, cap);
  if (!free_slot) { return rk_null; }
  bitset_set(data, cap, free_slot - 1);
  void* ptr = (char*)self + bitset_bytes(cap) + elsize * (free_slot - 1);
  rk_memcpy(ptr, obj, elsize);
  return ptr;
}
static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Spool_put(
    size_t elsize, size_t align, size_t cap, void* restrict self,
    void* restrict obj) {
  void* res = RK__Spool_try_put(elsize, align, cap, self, obj);
  RK_POOL_FAIL(res, self, rk_null, align, elsize);
  return res;
}
static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Dpool_try_put(
    size_t elsize, size_t align rk_unused, RK__pool_dynamic* restrict self,
    void* restrict obj) {
  if (!self->cap) { return rk_null; }
  size_t free_slot = bitset_first_trailing_zero(self->data, self->cap);
  if (!free_slot) { return rk_null; }
  bitset_set(self->data, self->cap, free_slot - 1);
  void* ptr = (char*)self->els + elsize * (free_slot - 1);
  return rk_memcpy(ptr, obj, elsize);
}
static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__Dpool_put(
    size_t elsize, size_t align, RK__pool_dynamic* restrict self,
    void* restrict obj) {
  void* res = RK__Dpool_try_put(elsize, align, self, obj);
  RK_POOL_FAIL(res, self, rk_null, align, elsize);
  return res;
}
#define RK__pool_try_put(self, el)                                             \
  RK__pool_dispatch(                                                           \
      self,                                                                    \
      RK__Spool_try_put(sizeof((self)->els[0]), alignof(RK__poolT(self)),      \
                        rk_COUNTOF((self)->RK__POOL_ELS), self,                \
                        (RK__poolT(self)[1]){el}),                             \
      RK__Dpool_try_put(sizeof((self)->els[0]), alignof(RK__poolT(self)),      \
                        (RK__pool_dynamic*)&((self)->RK__pooldummy),           \
                        (RK__poolT(self)[1]){el}))

#define RK__pool_put(self, el)                                                 \
  RK__pool_dispatch(                                                           \
      self,                                                                    \
      RK__Spool_put(sizeof((self)->els[0]), alignof(RK__poolT(self)),          \
                    rk_COUNTOF((self)->RK__POOL_ELS), self,                    \
                    (RK__poolT(self)[1]){el}),                                 \
      RK__Dpool_put(sizeof((self)->els[0]), alignof(RK__poolT(self)),          \
                    (RK__pool_dynamic*)&((self)->RK__pooldummy),               \
                    (RK__poolT(self)[1]){el}))

static_fun rk_forceinline void RK__Dpool_release(size_t elsize, size_t align,
                                                 RK__pool_dynamic* self) {
  if (!self->cap) { return; }
  alloc_deallocate(self->els, elsize * self->cap, align, self->alloc);
  alloc_delete(self->data, bitset_words(self->cap), self->alloc);
  self->cap = 0, self->data = 0, self->els = 0;
}

#define RK__pool_release(self)                                                 \
  RK__pool_dispatch(                                                           \
      self, *(self) = (typeof(*(self))){RK_ZINIT},                             \
      RK__Dpool_release(sizeof((self)->els[0]), alignof(RK__poolT(self)),      \
                        (RK__pool_dynamic*)&((self)->RK__pooldummy)))

#define RK__pool_foreach(self, it)                                             \
  for (typeof(self) RK___pool = (self); RK___pool; RK___pool = rk_null)        \
    for (size_t RK___cap = pool_cap(RK___pool), RK___i = (size_t)-1;           \
         (RK___i = bitset_next_set(RK___pool->data, RK___cap, RK___i))         \
         != (size_t)-1;)                                                       \
      for (RK__poolT(RK___pool)*const it         = RK___pool->els + RK___i,    \
                                      *RK___once = it;                         \
           RK___once; RK___once                  = 0)

#define RK__STATOVERLOAD__(m, ...)                                             \
  rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)
#define RK__STATOVERLOAD(m, ...)                                               \
  rk_CONC(m, rk_ARGCOUNT(__VA_ARGS__))(__VA_ARGS__)

/// @endcond

RK_HEADER_END
/// @}

#endif /* RK_POOL_H */
