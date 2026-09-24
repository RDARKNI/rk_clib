// SPDX-License-Identifier: MIT
/// @file rk_vec.h
/// @version 1.0
/// @defgroup rk_vec Vec (Dynamic Array) Interface
/// @brief Header file for a heap-allocated dynamic array (vec) implementation inspired by Sean
/// Barrett's stretchy buffer.
///
/// This file provides macros and functions for creating and managing dynamic arrays in C. The
/// implementation supports an optional custom allocator interface (defined in `rk_alloc.h`, turned
/// off via setting `RK_CUSTOM_ALLOCATORS` to `0`) for flexible memory management.
///
/// Each Vec is represented as a simple dynamically allocated pointer to the element type, which can
/// be dereferenced like a regular C array. Metadata such as the Vec's length and capacity are
/// stored in a header located just before the user-facing data pointer. This metadata can be
/// accessed via functions like `vec_count()`.
///
/// Elements can be added to the Vec using `vec_push()`, which automatically resizes the underlying
/// memory and updates the pointer as needed.
///
/// None of the macros are safe against multiple/unintuitive evaluation of arguments; never use any
/// expression with side effects as arguments of any of the macros.
///
/// A `NULL` pointer is considered a valid, empty Vec with zero capacity. Most functions (unless
/// otherwise stated) handle `NULL` vecs gracefully, automatically initializing them as needed
/// (e.g., `Vec(int) v = NULL; vec_push(v, 5)` initializes the Vec and adds the element 5). Once
/// initialized, vecs maintain the invariant that capacity is always >= length.
///
/// Features:
/// - Automatic resizing and capacity management
/// - Optional custom allocator support
/// - Safe and convenient macros for push, pop, clear, insert, erase, and more
/// - Iteration macros for easy looping over Vec elements
///
/// Usage:
/// ```c
/// Vec(int) vec = vec_init(int, 10);  // Create Vec with initial capacity 10
/// vec_push(vec, 5);                  // Add element 5 to vec
/// rk_assert(vec[0] == 5);
/// vec_release(vec);                  // Free Vec memory
/// ```
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{

#ifndef RK_VEC_H
#define RK_VEC_H
#include "rk_alloc.h"
RK_HEADER_BEGIN

/// @brief Macro to indicate that an object is a Vec.
/// @param T The type of elements stored in the Vec
/// @note Vec(void) indicates that a Vec of any type is accepted as a parameter, this does not hold
/// for macros which need type information. Using Vec as `Vec(void)` in any macro is undefined as
/// most vec macros rely on type information such as sizeof.
/// @note Implemented as `typeof(T*)` rather than `T*` so that array types such as `Vec(int[5])` are
/// supported.
#define Vec(T) typeof(T*)

/// @brief Header of a dynamically allocated Vec. The Vec is implemented as a contiguous block of
/// memory with a header (`VecHeader`) that stores metadata about the vec, such as its capacity,
/// length, and allocator.
typedef struct VecHeader {
#if RK_CUSTOM_ALLOCATORS
  Allocator alloc; ///< Allocator (can be disabled)
#endif
  size_t                    cap;    ///< Capacity of the Vec (in terms of elements)
  size_t                    count;  ///< Length of the Vec (in terms of elements)
  alignas_max unsigned char data[]; ///< Vec Data
} VecHeader;

/// @defgroup rk_vec_accessors_u Unchecked Vec Accessors
/// @ingroup rk_vec
/// @brief Unchecked Accessor functions and macros for Vec metadata. These macros do not perform
/// null-checks on the Vec and allow direct, unchecked member access.
///
/// @{

/// @brief Unchecked access to the header of `self` as an lvalue.
#define vec_HEADER(self)                                                                           \
  ((VecHeader*)(void*)((char*)(self)                                                               \
                       - offsetof(VecHeader, data))) // NOLINT(clang-analyzer-security.ArrayBound)

#if RK_CUSTOM_ALLOCATORS
# define vec_ALLOCATOR(V) (vec_HEADER(V)->alloc) // NOLINT(clang-analyzer-security.ArrayBound)

#else
# define vec_ALLOCATOR(V) ((void)(V), alloc_ctx)
#endif

/// @brief Unchecked access to the capacity of `self` as an lvalue.
#define vec_CAP(self)   (vec_HEADER(self)->cap) // NOLINT(clang-analyzer-security.ArrayBound)

/// @brief Unchecked access to the count of `self` as an lvalue.
#define vec_COUNT(self) (vec_HEADER(self)->count) // NOLINT(clang-analyzer-security.ArrayBound)
#define vec_LEN         vec_COUNT                 // NOLINT(clang-analyzer-security.ArrayBound)

/// @}

#define rk_ensure_vec_align(T)                                                                     \
  (void)static_assert_expr(alignof(T) <= align_max, "Over-aligned Types not supported.")

/// @brief `Vec(T) vec_init(T, size_t cap, Allocator alloc = alloc_ctx)`
/// - Initialises a Vec from an initial capacity and an optional Allocator.
/// @param T           The desired type of the Vec's elements
/// @param init_cap    The initial capacity of the Vec (in elements)
/// @param allocator   Optional allocator; defaults to `alloc_ctx`.
/// @return Vec(T) the vec
/// @note Zero-Capacity vecs are always uninitialised
///
/// Usage:
/// ```c
/// Vec(int) v1  = vec_init(int, 10);           // create an int-Vec with 10 cap
///                                                using alloc_ctx
/// Vec(int) v2 = vec_init(int, 2, my_alloc);   // creates an int Vec with 2 cap
///                                             // using my_alloc as allocator
/// Vec(int) v3 = vec_init(int, 0);             // does nothing (0 cap)
/// ```
#define vec_init(T, init_cap, ...)                                                                 \
  ((Vec(T))(rk_ensure_vec_align(T), rk_overload(RK__vec_init, T, init_cap, ##__VA_ARGS__)))

/// @brief `Vec(T) vec_init_list(T, Allocator alloc = alloc_ctx, T... values)` - Initialises a Vec
/// from a list of values.
/// @param T    The Type
/// @param alloc Allocator optional, defaults to alloc_ctx
/// @param values T, ... the values to initialise the Vec with
/// @return Vec(T) a new Vec, initialised with the values
///
/// Usage:
/// ```c
/// Vec(int) vec = vec_init(int, 1, 2, 3, 4, 5); /* uses alloc_ctx */
/// Vec(int) vec2 = vec_init(int, my_alloc, 1, 2, 3); /* uses my_alloc */
///
/// // Compound literals must be wrapped in parens
/// typedef struct Pair{ int x, y; } Pair;
/// Vec(struct Pair) vec3 = vec_init(Pair, ((Pair){1, 2}), ((Pair){3, 4}));
/// ```
#define vec_init_list(T, ...)                                                                      \
  ((Vec(T))(rk_ensure_vec_align(T), RK__vec_init_list(T, ##__VA_ARGS__)))

/// @brief `Vec(T) vec_copy(Vec(T) vec, Allocator alloc = alloc_ctx)` - Clones `vec`, copying its
/// contents. This macro creates a new Vec by copying the contents, allocating memory using either a
/// provided allocator or the first Vec's original allocator. The new Vec will have the same length,
/// and will contain copies of the elements currently in use.
/// @param vec       The Vec to copy
/// @param alloc     Allocator Optional, defaults to allocator of vec
/// @return A Vec containing a copy of the contents of vec, NULL if Vec is NULL
/// @note The new vec might have a different capacity than the original, but will have the same
/// length.
#define vec_copy(vec, ...) ((typeof(vec))rk_overload(RK__vec_copy, vec, ##__VA_ARGS__))
// todo copy semantics for allocators

/// @brief `void vec_release(Vec(T)& self)` - Frees the underlying allocation and sets the Vec to
/// NULL.
#define vec_release(self)  ((void)RK__vec_release(self))

/// @defgroup rk_vec_accessors_c Checked Vec Accessors
/// @ingroup rk_vec
/// @brief Checked Accessor functions and macros for Vec metadata. These macros check for
/// unallocated (NULL) Vecs.
///
/// @{

/// @brief Returns a pointer to the header of the vec, for direct access to metadata
/// @return Pointer to the header of the Vec or NULL, if `self` is NULL
static_fun rk_const VecHeader* vec_header(const Vec(void) self) {
  return self ? vec_HEADER(self) : rk_null;
}

/// @brief Returns the allocator of the vec or `alloc_ctx` if `self` is `NULL`.
static_fun rk_pure Allocator vec_allocator(const Vec(void) self) {
#if RK_CUSTOM_ALLOCATORS
  return self ? vec_ALLOCATOR(self) : alloc_ctx;
#else
  return (void)self, alloc_ctx;
#endif
}

/// @brief Returns the current capacity of the Vec, 0 iff `self` is NULL.
static_fun rk_pure size_t vec_cap(const Vec(void) self) { return self ? vec_CAP(self) : 0; }

/// @brief Returns the number of elements in the vec, 0 if `self` is NULL.
static_fun rk_pure size_t vec_count(const Vec(void) self) { return self ? vec_COUNT(self) : 0; }

/// @brief Alias for `vec_count()`
static_fun rk_pure size_t vec_len(const Vec(void) self) { return vec_count(self); }

/// @brief `size_t vec_allocation_size(Vec(T) self)` - Returns the total size of memory allocated
/// for the Vec in bytes, including. its header, 0 iff `self` is NULL.
#define vec_allocation_size(self) RK__vec_allocation_size(self, sizeof(*(self)))

/// @brief Returns the remaining count of elements that can be pushed to a vec without reallocation.
static_fun rk_pure size_t vec_remaining(const Vec(void) self) {
  return self ? vec_CAP(self) - vec_COUNT(self) : 0;
}

/// @brief Returns whether the count of a Vec is zero.
static_fun rk_pure bool vec_is_empty(const Vec(void) self) { return vec_count(self) == 0; }

/// @brief Returns whether an index is within the range of a Vec.
static_fun bool         vec_index_in_range(const Vec(void) self, size_t idx) {
  return idx < vec_count(self);
}

/// @brief Clears the contents of `self` by setting its count to 0.
static_fun void vec_clear(Vec(void) self) {
  if (self) { vec_COUNT(self) = 0; }
}

/// @brief `void vec_push(Vec(T)& self, T obj)` - Pushes a value onto the Vec, resising the
/// allocation, if necessary.
/// @attention **`obj` must not modify the vec due to sequencing issues**
/// @note Reassigns `self`, if necessary
#define vec_push(self, obj)           ((void)RK__vec_push(self, obj)) // NOLINT

/// @brief `void vec_push_n(Vec(T)& self, T* arr, size_t count)` - Copies `count` values of `arr`
/// onto `self`. `arr` must be a pointer variable of type `T*`.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary
#define vec_push_n(self, arr, count)  ((void)RK__vec_push_arr(self, arr, count))

/// @brief `void vec_push_unchecked(Vec(T)& self, T obj)` - Pushes a value onto the Vec, not
/// checking for capacity.
/// @attention **`obj` must not modify the vec due to sequencing issues**
#define vec_push_unchecked(self, obj) ((void)(RK__vec_push_u(self, obj)))

/// @brief `T vec_pop(Vec(T)& self)` - Pops the last value off the Vec and decreases its length.
/// @return The popped value
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour in case of empty or uninitialised Vec is undefined
#define vec_pop(self)                 ((self)[--vec_COUNT(RK__check_vec_pop(self))])

/// @brief `T* vec_pop(Vec(T)& self, size_t count)` - Pops 'count' values off the Vec, decreasing
/// its length.
/// @return A pointer to the popped memory region, to copy away from
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour in case of `count >= vec_count(self)` undefined
#define vec_pop_n(self, count)        (typeof(self))RK__vec_pop_n(sizeof(*(self)), self, count)

/// @brief `T& vec_front(Vec(T) self)` - Returns an Lvalue reference to the first element of the
/// Vec.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour undefined for empty or uninitialised vec
#define vec_front(self)               (((typeof(self))RK__vec_check_front(self))[0])

/// @brief `T& vec_back(Vec(T) self)` - Returns an Lvalue reference to the last element of the Vec.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour undefined for empty or uninitialised vec
#define vec_back(self)                (*((typeof(self))RK__vec_check_back(sizeof(*(self)), self)))

/// @brief `void vec_reserve(Vec(T)& self, size_t new_cap)` - Grows the Vec to be able to hold at
/// least `new_cap` elements.
/// @attention **Arguments with side effects are not safe in `vec_` macros**
/// @note Reassigns `self`, if necessary
#define vec_reserve(self, new_cap)    ((void)RK__vec_reserve(self, new_cap))

/// @brief `void vec_resize(Vec(T)& self, size_t len)` - Resizes the Vec to `len`, expanding the
/// capacity by reallocating and creating uninitialised objects if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary
#define vec_resize(self, len)         ((void)RK__vec_resize(self, len))

/// @brief `void vec_shrink_to_fit(Vec(T)& self)` - Shrink the Vec's capacity to be equal to its
/// length.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary; deallocates the Vec if it is empty.
#define vec_shrink_to_fit(self)       ((void)RK__vec_shrink_to_fit(self))

/// @brief `void vec_assign(Vec(T)& self, T* arr, size_t count)` - Assigns `count` objects of `arr`
/// to the Vec, overriding its contents and expanding `self`, if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary.
#define vec_assign(self, arr, count)  ((void)RK__vec_assign(self, arr, count))

/// @brief `void vec_insert_at(Vec(T)& self, size_t idx, T obj)` - Inserts an object at index `idx`,
/// shifting subsequent elements back and expanding `self`, if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary. Behavior is undefined if `idx` is out of bounds.
#define vec_insert_at(self, idx, obj) ((void)(RK__vec_insert_at(self, idx, obj)))

/// @brief `void vec_insert_at_unordered(Vec(T)& self, size_t idx, T obj)` - Inserts `obj` at index
/// `idx` without preserving element order. The element currently at `idx` is moved to the back
/// before `obj` is placed at `idx`. O(1) (ignoring possible reallocation), unlike `vec_insert_at`
/// which is O(n).
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary. Behavior is undefined if `idx` > vec_count(self).
#define vec_insert_at_unordered(self, idx, obj) ((void)RK__vec_insert_at_unordered(self, idx, obj))

/// @brief `void vec_insert_arr_at(Vec(T)& self, size_t idx, T* obj, size_t count)` - Batched
/// `vec_insert()`, faster when adding multiple elements at once. TODO SELF INSERTION
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @param self  The Vec (must be an lvalue)
/// @param idx   The Index of the Vec to store in
/// @param ptr   A pointer to the array of objects to insert
/// @param count The amount of objects to push
/// @code Vec(int) vec = vec_init(int, 10); int arr[3] = {1, 2, 3}; vec_insert_arr_at(vec, 1, arr,
/// countof(arr)); // Alternatively, pass a compound literal enclosed in parens such as
/// vec_insert_arr_at(vec, 2, ((int[]){4, 5, 6}), countof((int[]){4, 5, 6})); rk_assert(vec[0] ==
/// 1);
/// @endcode
/// @note Behavior is undefined if `idx` > vec_count(self)
#define vec_insert_arr_at(self, idx, ptr, count)                                                   \
  ((void)(RK__vec_insert_arr_at(self, idx, ptr, count)))

/// @brief `void vec_erase_at(Vec(T) self, size_t idx)` - Erases an element from `self` at index
/// `idx`, shifting subsequent elements forward.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` is out of bounds.
#define vec_erase_at(self, idx) ((void)(RK__vec_erase_at(self, idx)))

/// @brief `void vec_erase_at_unordered(Vec(T) self, size_t idx)` - Erases an element at index `idx`
/// without preserving element order. The last element is moved into the erased slot. O(1), unlike
/// `vec_erase_at` which is O(n).
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` is out of bounds.
#define vec_erase_at_unordered(self, idx)                                                          \
  ((void)(((self)[idx] = (self)[RK__decrease_index_check(self)])))

/// @brief `void vec_erase_at_n(Vec(T) self, size_t idx, size_t count)` - Erases `count` elements at
/// index `idx` from `self`, shifting subsequent elements forward.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` + count > vec_count()
#define vec_erase_at_n(self, idx, count) ((void)(RK__vec_erase_at_n(self, idx, count)))

/// @brief `T* vec_end(Vec(T) self)` - Returns a pointer one past the end of a the elements of
/// `self` or `NULL` if `self` is `NULL`.
#define vec_end(self)                    ((self) ? ((self) + vec_COUNT(self)) : (self))

/// @brief Convenience Macro to loop over the elements of a vec.
/// @param vec The Vec to loop over
/// @param it  The name of the iterator (pointer to each element)
/// @note Do not erase or add elements while looping in this fashion. Use `vec_iterate()` in that
/// case.
///
/// Usage:
/// ```c
/// Vec(int) vec = vec_init(int, 10);
/// vec_push(vec, 1), vec_push(vec, 2);
/// vec_foreach(vec, it) { printf("%d\n", *it); }
/// ```
#define vec_foreach(vec, it)                                                                       \
  for (typeof(*(vec))*RK___VEC = (vec), *const RK___END = vec_end(RK___VEC); RK___VEC != RK___END; \
       ++RK___VEC)                                                                                 \
    for (typeof(*RK___VEC)*const it = RK___VEC, *RK___ONCE = RK___VEC; RK___ONCE; RK___ONCE = 0)

/// @brief Like vec_foreach(), iterating in reverse order.
#define vec_foreach_reversed(vec, it)                                                              \
  for (typeof(*(vec))*const RK___VEC = (vec), *RK___END = vec_end(RK___VEC);                       \
       RK___VEC != RK___END;)                                                                      \
    for (typeof(*RK___VEC)*const it = --RK___END, *RK___ONCE = it; RK___ONCE; RK___ONCE = 0)

/// @brief Convenience Macro to erase all elements in a Vec that satisfy a predicate.
/// @param vec         The Vec to loop over
/// @param it          The name of the iterator (access via *it)
/// @param pred        The predicate (an expression working on *it)
///
/// Usage:
/// ```c
/// Vec(int) vec = vec_init(int, 10);
/// vec_push(vec, 1), vec_push(vec, 2);
/// vec_erase_if(vec, it, *it % 2); // remove even numbers
/// ```
#define vec_erase_if(vec, it, pred)                                                                \
  do {                                                                                             \
    typeof(vec) const RK___VEC = (vec);                                                            \
    if (!vec_count(RK___VEC)) { break; }                                                           \
    typeof(vec)            RK___BEG = RK___VEC;                                                    \
    const typeof(RK___VEC) RK___END = RK___BEG + vec_COUNT(RK___BEG);                              \
    for (typeof(RK___BEG) RK___IT = RK___VEC; RK___IT != RK___END; ++RK___IT) {                    \
      typeof(RK___VEC) const it = RK___IT;                                                         \
      if (!(pred)) { *RK___BEG++ = *RK___IT; }                                                     \
    }                                                                                              \
    vec_COUNT(RK___VEC) = (size_t)(RK___BEG - RK___VEC);                                           \
  } while (0)

/// @brief Reverse the elements of a Vec in place.
#define vec_reverse(vec)                                                                           \
  do {                                                                                             \
    typeof(vec) RK___BEG = (vec);                                                                  \
    if (!vec_count(RK___BEG)) { break; }                                                           \
    typeof(RK___BEG) RK___END = RK___BEG + vec_COUNT(RK___BEG) - 1;                                \
    for (; RK___BEG < RK___END; ++RK___BEG, --RK___END) { rk_SWAP(*RK___BEG, *RK___END); }         \
  } while (0)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

static_fun rk_forceinline size_t RK__decrease_index_check(void* self) {
  rk_assert(vec_count(self) && "Cannot decrease count of empty vec");
  return --vec_COUNT(self);
}

static_fun rk_forceinline void* RK__check_vec_push_u(void* self) {
  rk_assert(vec_remaining(self) && "Not enough capacity for unchecked push");
  return self;
}

static_fun rk_forceinline void* RK__check_vec_pop(void* self) {
  rk_assert(vec_count(self) && "Attempting to pop from zero-length vec");
  return self;
}

static_fun rk_forceinline void* RK__vec_pop_n(size_t elsize, void* self, size_t count) {
  rk_assert(vec_count(self) >= count && "Attempting to pop more than vec_count() elements");
  vec_COUNT(self) -= count;
  return (char*)self + rk_mult(elsize, vec_COUNT(self));
}

static_fun rk_forceinline void* RK__vec_check_front(void* self) {
  rk_assert(vec_count(self) && "Attempting to access front of zero-sized vec");
  return self;
}

static_fun rk_forceinline void* RK__vec_check_back(size_t elsize, void* self) {
  rk_assert(vec_count(self) && "Attempting to access back of zero-sized vec");
  return (char*)self + rk_mult(elsize, vec_COUNT(self) - 1);
}

static_fun rk_forceinline size_t RK__vec_assert_insertbounds(void* self, size_t i) {
  rk_assert(i <= vec_count(self) && "Attempting to insert into Vec at out of bounds index");
  return i;
}

static_fun rk_forceinline size_t RK__vec_assert_erasebounds_n(void* self, size_t i, size_t n) {
  rk_assert((i <= vec_count(self) && n <= vec_count(self) - i)
            && "Attempting to erase from Vec at out of bounds index");
  return i;
}

//  logical size of a vec if type information not available
#define RK__VECSIZE_UT(elsize, elcount) (offsetof(VecHeader, data) + rk_mult(elsize, elcount))
#define RK__VEC_COMPUTE_SIZE(V, C)      RK__VECSIZE_UT(sizeof(*(V)), C)
#define vec_ALLOCATION_SIZE(V)          RK__VEC_COMPUTE_SIZE(V, vec_CAP(V))
#define vec_LOGICAL_SIZE(V)             RK__VEC_COMPUTE_SIZE(V, vec_COUNT(V))

static_fun rk_forceinline rk_pure size_t RK__vec_allocation_size(const Vec(void) self,
                                                                 size_t          elsize) {
  return self ? RK__VECSIZE_UT(elsize, vec_CAP(self)) : 0;
}

static_fun rk_forceinline VecHeader* rk_alloc_size(3)
    RK__vec_init_f(size_t elsize, size_t init_cap, size_t total_size rk_unused,
                   size_t init_count RK_IFALLOC(, Allocator alloc)) {
  rk_assert_allocator_valid(alloc);
  (void)elsize; // todo
  VecHeader* v = (VecHeader*)alloc_allocate(total_size, align_max RK_IFALLOC(, alloc));
  v->cap = init_cap, v->count = init_count;
  RK_IFALLOC(v->alloc = alloc;)
  return v;
}
#define RK__VEC_NEW_NONZERO(T, cap, count, alloc)                                                  \
  ((typeof(T)*)(void*)(RK__vec_init_f(sizeof(T), cap,                                              \
                                      offsetof(VecHeader, data) + sizeof_n(T, cap),                \
                                      count RK_IFALLOC(, alloc))                                   \
                           ->data))
#define RK__VEC_NEW(T, cap, count, alloc)                                                          \
  ((cap) ? RK__VEC_NEW_NONZERO(T, cap, count, alloc) : rk_null)

// initialises a Vec with positive cap (no cap 0 check) and assigns it to V
#define RK__VEC_INIT_ASSIGN(V, C, A) ((V) = RK__VEC_NEW_NONZERO(*(V), (C), 0, (A)))

#define RK__vec_init(T, C, A)        RK__VEC_NEW(T, (C), 0, (A))
#define RK__vec_init3(T, C, A)       rk_disable_if(RK__vec_init(T, C, A))
#define RK__vec_init2(T, C)          RK__vec_init(T, C, alloc_ctx)

#define RK__vec_copy(V, A)                                                                         \
  ((V) ? rk_copy(RK__VEC_NEW(*(V), vec_COUNT(V), vec_COUNT(V), (A)), (V), vec_COUNT(V)) : rk_null)

#define RK__vec_copy2(V, A) rk_disable_if(RK__vec_copy(V, A))
#define RK__vec_copy1(V)    RK__vec_copy(V, vec_ALLOCATOR(V))

#define RK__vec_release(V)                                                                         \
  ((V)                                                                                             \
   && (alloc_deallocate(vec_HEADER(V), vec_ALLOCATION_SIZE(V),                                     \
                        align_max RK_IFALLOC(, vec_ALLOCATOR(V))),                                 \
       (V) = rk_null))

/// always reallocates to a positive cap, sets cap accordingly
#define RK__VEC_CHANGE_CAP(V, C)                                                                   \
  ((V)                                                                                             \
   = (typeof(V))(void*)(((VecHeader*)alloc_reallocate(vec_HEADER(V), vec_ALLOCATION_SIZE(V),       \
                                                      RK__VEC_COMPUTE_SIZE(V, C),                  \
                                                      align_max RK_IFALLOC(, vec_ALLOCATOR(V))))   \
                            ->data),                                                               \
   vec_CAP(V) = (C), (V))

/*doubles capacity if at limit*/
#define RK__vec_reserve_1(V)                                                                       \
  ((V) ? (vec_COUNT(V) == vec_CAP(V) ? RK__VEC_CHANGE_CAP(V, rk_mult(vec_CAP(V), 2)) : (V))        \
       : RK__VEC_INIT_ASSIGN(V, 1, alloc_ctx))

#define RK__vec_reserve(V, C)                                                                      \
  ((V) ? ((C) > vec_CAP(V) ? RK__VEC_CHANGE_CAP(V, C) : (V))                                       \
       : ((C) ? RK__VEC_INIT_ASSIGN(V, C, alloc_ctx) : rk_null))

#define RK__vec_resize(V, C) (RK__vec_reserve(V, C), (V) && (vec_COUNT(V) = (C)))

#define RK__vec_shrink_to_fit(V)                                                                   \
  ((V) && vec_COUNT(V) < vec_CAP(V)                                                                \
   && (vec_COUNT(V) ? RK__VEC_CHANGE_CAP(V, vec_COUNT(V)) : (vec_release(V), (V) = rk_null)))

#define RK__vec_push_u(V, O)        ((V)[vec_COUNT(RK__check_vec_push_u(V))++] = (O))
#define RK__vec_push(V, O)          (RK__vec_reserve_1(V), RK__vec_push_u(V, O))

#define RK__vec_push_arr_u(V, O, N) (rk_copy((V) + vec_COUNT(V), O, N), vec_COUNT(V) += (N))
#define RK__vec_push_arr(V, O, N)                                                                  \
  ((void)((N) && (RK__vec_reserve(V, vec_count(V) + (N)), RK__vec_push_arr_u(V, O, N), 1)))

static_fun rk_forceinline void RK__vec_insert_arr_at_f(size_t elsize, void* restrict v, size_t i,
                                                       const void* restrict arr, size_t n) {
  size_t old_count = vec_COUNT(v); // NOLINT(clang-analyzer-security.ArrayBound)
  char * src = (char*)v + rk_mult(i, elsize), *dst = src + rk_mult(n, elsize);
  memmove(dst, src, rk_mult(old_count - i, elsize));
  memcpy(src, arr, rk_mult(elsize, n));
  vec_COUNT(v) = old_count + n; // NOLINT(clang-analyzer-security.ArrayBound)
}

#define RK__vec_insert_arr_at_u(V, I, O, N)                                                        \
  RK__vec_insert_arr_at_f(sizeof(*(V)), (V), RK__vec_assert_insertbounds(V, I), (O), (N))
#define RK__vec_insert_at_u(V, I, O) RK__vec_insert_arr_at_u(V, I, ((typeof (*(V))[1]){(O)}), 1)

#define RK__vec_insert_at(V, I, O)   (RK__vec_reserve_1(V), RK__vec_insert_at_u(V, I, O))

#define RK__vec_insert_arr_at(V, I, O, N)                                                          \
  ((void)((N) && (RK__vec_reserve(V, vec_count(V) + (N)), RK__vec_insert_arr_at_u(V, I, O, N), 1)))

static_fun rk_forceinline void RK__vec_insert_at_unordered_f(size_t elsize, void* restrict vec,
                                                             size_t i, const void* restrict o) {
  size_t count = vec_COUNT(vec); // NOLINT(clang-analyzer-security.ArrayBound)
  char*  dst   = (char*)vec + rk_mult(elsize, count);
  if (i < count) {
    char* src = (char*)vec + rk_mult(elsize, i);
    memcpy(dst, src, elsize);
    memcpy(src, o, elsize);
  } else {
    memcpy(dst, o, elsize);
  }
  ++vec_COUNT(vec); // NOLINT(clang-analyzer-security.ArrayBound)
}

#define RK__vec_insert_at_unordered(V, I, O)                                                       \
  (RK__vec_reserve_1(V),                                                                           \
   RK__vec_insert_at_unordered_f(sizeof(*(V)), V, RK__vec_assert_insertbounds(V, I),               \
                                 ((typeof (*(V))[1]){(O)})))

static_fun rk_forceinline void RK__vec_erase_at_n_f(size_t elsize, void* v, size_t i, size_t n) {
  if (!n) { return; }
  char *dst = (char*)v + rk_mult(i, elsize), *src = dst + rk_mult(n, elsize);
  memmove(dst, src,
          rk_mult(((vec_COUNT(v) -= n) - i),
                  elsize)); // NOLINT(clang-analyzer-security.ArrayBound)
}
#define RK__vec_erase_at_n(V, I, N)                                                                \
  RK__vec_erase_at_n_f(sizeof(*(V)), (V), RK__vec_assert_erasebounds_n(V, I, N), (N))
#define RK__vec_erase_at(V, I) RK__vec_erase_at_n(V, I, 1)

#define RK__vec_assign(V, O, N)                                                                    \
  (RK__vec_reserve(V, N), (V) && (vec_COUNT(V) = (N), rk_copy(V, O, N)))

// msvc sizeof returns 0
#define RK__vec_init_list_(T, arr, alloc)                                                          \
  memcpy(RK__VEC_NEW_NONZERO(T, rk_COUNTOF(arr), rk_COUNTOF(arr), alloc), arr, sizeof(arr))

#define RK__VEC_contrav(T, x) _Generic(x, T: x, Allocator: (T){RK_ZINIT})

#if RK_CUSTOM_ALLOCATORS
# define RK__vec_init_list(T, ...)                                                                 \
   _Generic(VA_FIRST(__VA_ARGS__),                                                                 \
       Allocator: RK__vec_init_list_(T, ((const T[]){VA_REST(__VA_ARGS__)}),                       \
                                     RK__contrav(Allocator, VA_FIRST(__VA_ARGS__))),               \
       default: RK__vec_init_list_(                                                                \
                T, ((const T[]){RK__VEC_contrav(T, VA_FIRST(__VA_ARGS__)), VA_REST(__VA_ARGS__)}), \
                alloc_ctx))
#else
# define RK__vec_init_list(T, ...)                                                                 \
   RK__vec_init_list_(                                                                             \
       T, ((const T[]){RK__VEC_contrav(T, VA_FIRST(__VA_ARGS__)), VA_REST(__VA_ARGS__)}), 0)
#endif

/// @endcond

RK_HEADER_END
/// @}
#endif // RK_VEC_H

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
