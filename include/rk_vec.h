// SPDX-License-Identifier: MIT
/// @file rk_vec.h
/// @version 1.0.0
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
RKI_HEADER_BEGIN

/// @brief Macro to indicate that an object is a Vec.
/// @param T The type of elements stored in the Vec
/// @note Vec(void) indicates that a Vec of any type is accepted as a parameter, this does not hold
/// for macros which need type information. Using Vec as `Vec(void)` in any macro is undefined as
/// most vec macros rely on type information such as sizeof.
/// @note Array element types (e.g. `Vec(int[5])`) are not supported, typedef'd or not:
/// `vec_push()`, `vec_insert_at`, and every other operation that places a new element relies on
/// plain C assignment (`self[i] = value`), which C disallows for array types regardless of how
/// `Vec(T)` itself expands or whether the array type has a name of its own.
#define Vec(T) T*

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
  ((Vec(T))((void)static_assert_expr(alignof(T) <= align_max,                                      \
                                     "Over-aligned Types not supported."),                         \
            RKI_OVERLOAD(RKI_VEC_INIT, T, init_cap, ##__VA_ARGS__)))

/// @brief `Vec(T) vec_init_list(T, Allocator alloc = alloc_ctx, T... values)` - Initialises a Vec
/// from a list of values.
/// @param T    The Type
/// @param alloc Allocator optional, defaults to alloc_ctx
/// @param values T, ... the values to initialise the Vec with
/// @return Vec(T) a new Vec, initialised with the values
///
/// Usage:
/// ```c
/// Vec(int) vec = vec_init_list(int, 1, 2, 3, 4, 5); /* uses alloc_ctx */
/// Vec(int) vec2 = vec_init_list(int, my_alloc, 1, 2, 3); /* uses my_alloc */
///
/// // Compound literals must be wrapped in parens
/// typedef struct Pair { int x, y; } Pair;
/// Vec(struct Pair) vec3 = vec_init_list(Pair, ((Pair){1, 2}), ((Pair){3, 4}));
/// ```
#define vec_init_list(T, ...)                                                                      \
  ((Vec(T))((void)static_assert_expr(alignof(T) <= align_max,                                      \
                                     "Over-aligned Types not supported."),                         \
            RKI_VEC_INIT_LIST(T, ##__VA_ARGS__)))

/// @brief `Vec(T) vec_from(T* arr, size_t count, Allocator alloc = alloc_ctx)` - Constructs a new
/// Vec by copying `count` elements from `arr`.
/// @param arr   Source array of `count` elements; its element type becomes the new Vec's element
/// type (via `typeof(*arr)`)
/// @param count Number of elements to copy
/// @param alloc Allocator Optional, defaults to `alloc_ctx`
/// @return A Vec containing a copy of `arr`'s first `count` elements, or `NULL` if `count == 0`
/// @note To clone an existing Vec while preserving its own allocator, pass it directly along with
/// its own count/allocator: `vec_from(v, vec_count(v), vec_allocator(v))`. Unlike a Vec, a plain
/// array has no allocator of its own to default to, so `vec_from()` always defaults to `alloc_ctx`
/// when no allocator is given.
///
/// Usage:
/// ```c
/// int arr[] = {1, 2, 3};
/// Vec(int) v = vec_from(arr, 3);            // uses alloc_ctx
/// Vec(int) v2 = vec_from(arr, 3, my_alloc);  // uses my_alloc
/// ```
#define vec_from(arr, count, ...)                                                                  \
  ((typeof(*(arr))*)RKI_OVERLOAD(RKI_VEC_FROM, arr, count, ##__VA_ARGS__))

/// @brief `void vec_release(Vec(T)& self)` - Frees the underlying allocation and sets the Vec to
/// NULL.
#define vec_release(self) ((void)RKI_VEC_RELEASE(self))

/// @brief Returns the number of elements in the vec, 0 if `self` is NULL.
rklib_fun rk_pure size_t vec_count(const Vec(void) self);
#define vec_COUNT(self) rk_to_rvalue(RKI_VEC_COUNT(self))

/// @brief Alias for `vec_count()`
rklib_fun rk_pure size_t vec_len(const Vec(void) self);
#define vec_LEN(self) vec_COUNT(self)

/// @brief Returns the current capacity of the Vec, 0 iff `self` is NULL.
rklib_fun rk_pure size_t vec_cap(const Vec(void) self);
#define vec_CAP(self) rk_to_rvalue(RKI_VEC_CAP(self))

/// @brief Returns the Allocator the Vec was constructed with, or `alloc_ctx` if `self` is `NULL`
/// or custom allocators are disabled.
rklib_fun rk_pure Allocator vec_allocator(const Vec(void) self);
#define vec_ALLOCATOR(self) rk_to_rvalue(RKI_VEC_ALLOCATOR(self))

/// @brief Returns whether the count of a Vec is zero.
rklib_fun rk_pure bool vec_is_empty(const Vec(void) self);

/// @brief Clears the contents of `self` by setting its count to 0.
rklib_fun void         vec_clear(Vec(void) self);

/// @brief `size_t vec_allocation_size(Vec(T) self)` - Returns the total size of memory allocated
/// for the Vec in bytes, including its header, 0 iff `self` is NULL.
#define vec_allocation_size(self) RKI_VEC_ALLOCATION_SIZE(self)

/// @brief Returns the remaining count of elements that can be pushed to a vec without reallocation.
rklib_fun rk_pure size_t vec_remaining(const Vec(void) self);

/// @brief Returns whether an index is within the range of a Vec.
rklib_fun rk_pure bool   vec_index_in_range(const Vec(void) self, size_t idx);

/// @brief `void vec_reserve(Vec(T)& self, size_t new_cap)` - Grows the Vec to be able to hold at
/// least `new_cap` elements.
/// @attention **Arguments with side effects are not safe in `vec_` macros**
/// @note Reassigns `self`, if necessary
#define vec_reserve(self, new_cap)    ((void)RKI_VEC_RESERVE(self, new_cap))

/// @brief `void vec_resize(Vec(T)& self, size_t len)` - Resizes the Vec to `len`, expanding the
/// capacity by reallocating and creating uninitialised objects if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary
#define vec_resize(self, len)         ((void)RKI_VEC_RESIZE(self, len))

/// @brief `void vec_shrink_to_fit(Vec(T)& self)` - Shrinks the Vec's capacity to the next power of
/// two greater than or equal to its length (matching `str_shrink_to_fit()`'s convention), leaving
/// some slack to reduce reallocation on subsequent growth.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary; deallocates the Vec if it is empty.
/// @note Use `vec_shrink_to_fit_exact()` for an exact-capacity shrink.
#define vec_shrink_to_fit(self)       ((void)RKI_VEC_SHRINK_TO_FIT(self))

/// @brief `void vec_shrink_to_fit_exact(Vec(T)& self)` - Shrinks the Vec's capacity to be exactly
/// equal to its length.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary; deallocates the Vec if it is empty.
#define vec_shrink_to_fit_exact(self) ((void)RKI_VEC_SHRINK_TO_FIT_EXACT(self))

/// @brief `void vec_assign(Vec(T)& self, T* arr, size_t count)` - Assigns `count` objects of `arr`
/// to the Vec, overriding its contents and expanding `self`, if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @attention `arr[0..count)` must not overlap the Vec's own backing allocation: if growth is
/// triggered, the old buffer is freed before the copy from `arr` happens, turning an `arr` that
/// points into it into a use-after-free; even without growth, the underlying copy is a plain
/// `memcpy`, which is undefined for overlapping source and destination.
/// @note Reassigns `self`, if necessary.
#define vec_assign(self, arr, count)  ((void)RKI_VEC_ASSIGN(self, arr, count))

// todo docs
#define vec_begin(self)               ((typeof(self))(self))

/// @brief `T* vec_end(Vec(T) self)` - Returns a pointer one past the end of a the elements of
/// `self` or `NULL` if `self` is `NULL`.
#define vec_end(self)                 ((self) ? ((self) + RKI_VEC_COUNT(self)) : (self))

/// @brief `T& vec_front(Vec(T) self)` - Returns an Lvalue reference to the first element of the
/// Vec.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour undefined for empty or uninitialised vec
#define vec_front(self)               (((typeof(self))rki_vec_check_front(self))[0])

/// @brief `T& vec_back(Vec(T) self)` - Returns an Lvalue reference to the last element of the Vec.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour undefined for empty or uninitialised vec
#define vec_back(self)                (*((typeof(self))rki_vec_check_back(sizeof(*(self)), self)))

/// @brief `void vec_push(Vec(T)& self, T obj)` - Pushes a value onto the Vec, resising the
/// allocation, if necessary.
/// @attention **`obj` must not modify the vec due to sequencing issues**
/// @note Reassigns `self`, if necessary
#define vec_push(self, obj)           ((void)RKI_VEC_PUSH(self, obj)) // NOLINT

/// @brief `void vec_push_n(Vec(T)& self, T* arr, size_t count)` - Copies `count` values of `arr`
/// onto `self`. `arr` must be a pointer variable of type `T*`.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @attention `arr[0..count)` must not overlap the Vec's own backing allocation, for the same
/// reasons documented on `vec_assign()`.
/// @note Reassigns `self`, if necessary
#define vec_push_n(self, arr, count)  ((void)RKI_VEC_PUSH_N(self, arr, count))

/// @brief `void vec_push_unchecked(Vec(T)& self, T obj)` - Pushes a value onto the Vec, not
/// checking for capacity.
/// @attention **`obj` must not modify the vec due to sequencing issues**
#define vec_push_unchecked(self, obj) ((void)(RKI_VEC_PUSH_U(self, obj)))

/// @brief `T vec_pop(Vec(T)& self)` - Pops the last value off the Vec and decreases its length.
/// @return The popped value
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour in case of empty or uninitialised Vec is undefined
#define vec_pop(self)                 ((self)[--RKI_VEC_COUNT(rki_check_vec_pop(self))])

/// @brief `T* vec_pop(Vec(T)& self, size_t count)` - Pops 'count' values off the Vec, decreasing
/// its length.
/// @return A pointer to the popped memory region, to copy away from
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behaviour in case of `count > vec_count(self)` undefined
#define vec_pop_n(self, count)        (typeof(self))rki_vec_pop_n(sizeof(*(self)), self, count)

/// @brief `void vec_insert_at(Vec(T)& self, size_t idx, T obj)` - Inserts an object at index `idx`,
/// shifting subsequent elements back and expanding `self`, if necessary.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary. Behavior is undefined if `idx` is out of bounds.
#define vec_insert_at(self, idx, obj) ((void)(RKI_VEC_INSERT_AT(self, idx, obj)))

/// @brief `void vec_insert_at_unordered(Vec(T)& self, size_t idx, T obj)` - Inserts `obj` at index
/// `idx` without preserving element order. The element currently at `idx` is moved to the back
/// before `obj` is placed at `idx`. O(1) (ignoring possible reallocation), unlike `vec_insert_at`
/// which is O(n).
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Reassigns `self`, if necessary. Behavior is undefined if `idx` > vec_count(self).
#define vec_insert_at_unordered(self, idx, obj) ((void)RKI_VEC_INSERT_AT_UNORDERED(self, idx, obj))

/// @brief `void vec_insert_arr_at(Vec(T)& self, size_t idx, T* obj, size_t count)` - Batched
/// `vec_insert()`, faster when adding multiple elements at once.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @attention `ptr[0..count)` must not overlap the Vec's own backing allocation, for the same
/// reasons documented on `vec_assign()`.
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
  ((void)(RKI_VEC_INSERT_ARR_AT(self, idx, ptr, count)))

/// @brief `void vec_erase_at(Vec(T) self, size_t idx)` - Erases an element from `self` at index
/// `idx`, shifting subsequent elements forward.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` is out of bounds.
#define vec_erase_at(self, idx) ((void)(RKI_VEC_ERASE_AT(self, idx)))

/// @brief `void vec_erase_at_unordered(Vec(T) self, size_t idx)` - Erases an element at index `idx`
/// without preserving element order. The last element is moved into the erased slot. O(1), unlike
/// `vec_erase_at` which is O(n).
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` is out of bounds.
#define vec_erase_at_unordered(self, idx)                                                          \
  ((void)(((self)[idx] = (self)[rki_decrease_index_check(self)])))

/// @brief `void vec_erase_at_n(Vec(T) self, size_t idx, size_t count)` - Erases `count` elements at
/// index `idx` from `self`, shifting subsequent elements forward.
/// @attention **Arguments with side effects are not safe in vec_ macros**
/// @note Behavior is undefined if `idx` + count > vec_count()
#define vec_erase_at_n(self, idx, count) ((void)(RKI_VEC_ERASE_AT_N(self, idx, count)))

/// @brief Reverse the elements of a Vec in place.
#define vec_reverse(vec)                                                                           \
  do {                                                                                             \
    typeof(vec) RKI__BEG = (vec);                                                                  \
    if (!vec_count(RKI__BEG)) { break; }                                                           \
    typeof(RKI__BEG) RKI__END = RKI__BEG + RKI_VEC_COUNT(RKI__BEG) - 1;                            \
    for (; RKI__BEG < RKI__END; ++RKI__BEG, --RKI__END) { rk_SWAP(*RKI__BEG, *RKI__END); }         \
  } while (0)

/// @brief Visits every element in index order.
/// @param vec The Vec to iterate. Evaluated once.
/// @param it  Iterator name (a pointer into the Vec; access via `*it`).
/// @note break stops traversal; continue advances to the next element.
///
/// Usage:
/// ```c
/// vec_foreach(vec, it) { printf("%d\n", *it); }
/// ```
#define vec_foreach(vec, it)          RKI_VEC_FOREACH(vec, it)

/// @brief Like `vec_foreach()`, but iterates in reverse index order. Same parameters and contract.
#define vec_foreach_reversed(vec, it) RKI_VEC_FOREACH_REVERSED(vec, it)

/// @brief Erases every element satisfying `pred`.
/// @param vec  The Vec to erase from. Evaluated once.
/// @param it   Iterator name (access via `*it`).
/// @param pred Predicate expression, evaluated once per original element.
/// @note The predicate must not structurally modify the Vec.
///
/// Usage:
/// ```c
/// Vec(int) vec = vec_init(int, 10);
/// vec_push(vec, 1), vec_push(vec, 2);
/// vec_erase_if(vec, it, *it % 2); // remove odd numbers
/// ```
#define vec_erase_if(vec, it, pred)   RKI_VEC_ERASE_IF(vec, it, pred)

#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

/// @brief Header of a dynamically allocated Vec. The Vec is implemented as a contiguous block of
/// memory with a header (`RKI_VecHdr`) that stores metadata about the vec, such as its capacity,
/// length, and allocator.
typedef struct RKI_VecHdr {
#if RK_CUSTOM_ALLOCATORS
  Allocator alloc; ///< Allocator (can be disabled)
#endif
  size_t                    cap;    ///< Capacity of the Vec (in terms of elements)
  size_t                    count;  ///< Length of the Vec (in terms of elements)
  alignas_max unsigned char data[]; ///< Vec Data
} RKI_VecHdr;

#define RKI_VEC_HDR(self)                                                                          \
  ((RKI_VecHdr*)(void*)((char*)(self)                                                              \
                        - offsetof(RKI_VecHdr,                                                     \
                                   data)))             // NOLINT(clang-analyzer-security.ArrayBound)
#define RKI_VEC_CAP(self)   (RKI_VEC_HDR(self)->cap)   // NOLINT(clang-analyzer-security.ArrayBound)
#define RKI_VEC_COUNT(self) (RKI_VEC_HDR(self)->count) // NOLINT(clang-analyzer-security.ArrayBound)
#define RKI_VEC_ALLOCATOR(self)                                                                    \
  RKI_allocatorof(RKI_VEC_HDR(self)) // NOLINT(clang-analyzer-security.ArrayBound)

rklib_fun rk_pure size_t vec_count(const Vec(void) self) { return self ? RKI_VEC_COUNT(self) : 0; }
rklib_fun rk_pure size_t vec_len(const Vec(void) self) { return vec_count(self); }
rklib_fun rk_pure size_t vec_cap(const Vec(void) self) { return self ? RKI_VEC_CAP(self) : 0; }
rklib_fun rk_pure bool   vec_is_empty(const Vec(void) self) { return vec_count(self) == 0; }
rklib_fun rk_pure Allocator vec_allocator(const Vec(void) self) {
  return self ? vec_ALLOCATOR(self) : alloc_ctx;
}
rklib_fun void vec_clear(Vec(void) self) {
  if (self) { RKI_VEC_COUNT(self) = 0; }
}
rklib_fun rk_pure size_t vec_remaining(const Vec(void) self) {
  return self ? RKI_VEC_CAP(self) - RKI_VEC_COUNT(self) : 0;
}

rklib_fun rk_pure bool vec_index_in_range(const Vec(void) self, size_t idx) {
  return idx < vec_count(self);
}

rklib_fun rk_forceinline size_t rki_decrease_index_check(void* self) {
  rk_assert(vec_count(self) && "Cannot decrease count of empty vec");
  return --RKI_VEC_COUNT(self);
}

rklib_fun rk_forceinline void* rki_check_vec_push_u(void* self) {
  rk_assert(vec_remaining(self) && "Not enough capacity for unchecked push");
  return self;
}

rklib_fun rk_forceinline void* rki_check_vec_pop(void* self) {
  rk_assert(vec_count(self) && "Attempting to pop from zero-length vec");
  return self;
}

rklib_fun rk_forceinline void* rki_vec_pop_n(size_t elsize, void* self, size_t count) {
  rk_assert(vec_count(self) >= count && "Attempting to pop more than vec_count() elements");
  RKI_VEC_COUNT(self) -= count;
  return (char*)self + rk_mult(elsize, RKI_VEC_COUNT(self));
}

rklib_fun rk_forceinline void* rki_vec_check_front(void* self) {
  rk_assert(vec_count(self) && "Attempting to access front of zero-sized vec");
  return self;
}

rklib_fun rk_forceinline void* rki_vec_check_back(size_t elsize, void* self) {
  rk_assert(vec_count(self) && "Attempting to access back of zero-sized vec");
  return (char*)self + rk_mult(elsize, RKI_VEC_COUNT(self) - 1);
}

rklib_fun rk_forceinline size_t rki_vec_assert_insertbounds(void* self, size_t i) {
  rk_assert(i <= vec_count(self) && "Attempting to insert into Vec at out of bounds index");
  return i;
}

rklib_fun rk_forceinline size_t rki_vec_assert_erasebounds_n(void* self, size_t i, size_t n) {
  rk_assert((i <= vec_count(self) && n <= vec_count(self) - i)
            && "Attempting to erase from Vec at out of bounds index");
  return i;
}

//  logical size of a vec if type information not available
#define RKI_VECSIZE_UT(elsize, elcount) (offsetof(RKI_VecHdr, data) + rk_mult(elsize, elcount))
#define RKI_VEC_COMPUTE_SIZE(V, C)      RKI_VECSIZE_UT(sizeof(*(V)), C)
#define RKI_VEC_ALLOCSIZE(V)            RKI_VEC_COMPUTE_SIZE(V, RKI_VEC_CAP(V))

#define RKI_VEC_ALLOCATION_SIZE(self)   rki_vec_allocation_size(self, sizeof(*(self)))
rklib_fun rk_forceinline rk_pure size_t rki_vec_allocation_size(const Vec(void) self,
                                                                size_t          elsize) {
  return self ? RKI_VECSIZE_UT(elsize, RKI_VEC_CAP(self)) : 0;
}

rklib_fun rk_forceinline RKI_VecHdr* rk_alloc_size(2)
    rki_vec_init(size_t init_cap, size_t total_size,
                 size_t init_count RK_IFALLOC(, Allocator alloc)) {
  RKI_assert_allocator_valid(alloc);
  RKI_VecHdr* v = (RKI_VecHdr*)alloc_allocate(total_size, align_max RK_IFALLOC(, alloc));
  v->cap = init_cap, v->count = init_count;
  RK_IFALLOC(v->alloc = alloc;)
  return v;
}
#define RKI_VEC_NEW_NONZERO(T, cap, count, alloc)                                                  \
  ((typeof(T)*)(void*)(rki_vec_init(cap, offsetof(RKI_VecHdr, data) + sizeof_n(T, cap),            \
                                    count RK_IFALLOC(, alloc))                                     \
                           ->data))
#define RKI_VEC_NEW(T, cap, count, alloc)                                                          \
  ((cap) ? RKI_VEC_NEW_NONZERO(T, cap, count, alloc) : rk_null)

// initialises a Vec with positive cap (no cap 0 check) and assigns it to V
#define RKI_VEC_INIT_ASSIGN_NONZERO(V, C, A) ((V) = RKI_VEC_NEW_NONZERO(*(V), (C), 0, (A)))

#define RKI_VEC_INIT(T, C, A)                RKI_VEC_NEW(T, C, 0, A)
#define RKI_VEC_INIT3(T, C, A)               RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_VEC_INIT(T, C, A))
#define RKI_VEC_INIT2(T, C)                  RKI_VEC_INIT(T, C, alloc_ctx)

#define RKI_VEC_FROM(arr, count, alloc)                                                            \
  ((count) ? rk_copy(RKI_VEC_NEW(*(arr), (count), (count), (alloc)), (arr), (count)) : rk_null)
#define RKI_VEC_FROM3(arr, count, alloc)                                                           \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(RKI_VEC_FROM(arr, count, alloc))
#define RKI_VEC_FROM2(arr, count) RKI_VEC_FROM(arr, count, alloc_ctx)

#define RKI_VEC_RELEASE(V)                                                                         \
  ((V)                                                                                             \
   && (alloc_deallocate(RKI_VEC_HDR(V), RKI_VEC_ALLOCSIZE(V),                                      \
                        align_max RK_IFALLOC(, RKI_VEC_ALLOCATOR(V))),                             \
       (V) = rk_null))

/// always reallocates to a positive cap, sets cap accordingly
#define RKI_VEC_CHANGE_CAP(V, C)                                                                   \
  ((V) = (typeof(V))(void*)(((RKI_VecHdr*)alloc_reallocate(                                        \
                                 RKI_VEC_HDR(V), RKI_VEC_ALLOCSIZE(V), RKI_VEC_COMPUTE_SIZE(V, C), \
                                 align_max RK_IFALLOC(, RKI_VEC_ALLOCATOR(V))))                    \
                                ->data),                                                           \
   RKI_VEC_CAP(V) = (C), (V))

/*doubles capacity if at limit*/
#define RKI_VEC_RESERVE_1(V)                                                                       \
  ((V) ? (RKI_VEC_COUNT(V) == RKI_VEC_CAP(V) ? RKI_VEC_CHANGE_CAP(V, rk_mult(RKI_VEC_CAP(V), 2))   \
                                             : (V))                                                \
       : RKI_VEC_INIT_ASSIGN_NONZERO(V, 1, alloc_ctx))

#define RKI_VEC_RESERVE(V, C)                                                                      \
  ((V) ? ((C) > RKI_VEC_CAP(V) ? RKI_VEC_CHANGE_CAP(V, C) : (V))                                   \
       : ((C) ? RKI_VEC_INIT_ASSIGN_NONZERO(V, C, alloc_ctx) : rk_null))

#define RKI_VEC_RESIZE(V, C) (RKI_VEC_RESERVE(V, C), (V) && (RKI_VEC_COUNT(V) = (C)))

#define RKI_VEC_SHRINK_TO_FIT_EXACT(V)                                                             \
  ((V) && RKI_VEC_COUNT(V) < RKI_VEC_CAP(V)                                                        \
   && (RKI_VEC_COUNT(V) ? RKI_VEC_CHANGE_CAP(V, RKI_VEC_COUNT(V))                                  \
                        : (vec_release(V), (V) = rk_null)))

#define RKI_VEC_SHRINK_TO_FIT(V)                                                                   \
  ((V) && (RKI_VEC_COUNT(V) ? stdc_bit_ceil(RKI_VEC_COUNT(V)) : 0) < RKI_VEC_CAP(V)                \
   && (RKI_VEC_COUNT(V) ? RKI_VEC_CHANGE_CAP(V, stdc_bit_ceil(RKI_VEC_COUNT(V)))                   \
                        : (vec_release(V), (V) = rk_null)))

#define RKI_VEC_PUSH_U(V, O)      ((V)[RKI_VEC_COUNT(rki_check_vec_push_u(V))++] = (O))
#define RKI_VEC_PUSH(V, O)        (RKI_VEC_RESERVE_1(V), RKI_VEC_PUSH_U(V, O))

#define RKI_VEC_PUSH_N_U(V, O, N) (rk_copy((V) + RKI_VEC_COUNT(V), O, N), RKI_VEC_COUNT(V) += (N))
#define RKI_VEC_PUSH_N(V, O, N)                                                                    \
  ((void)((N) && (RKI_VEC_RESERVE(V, vec_count(V) + (N)), RKI_VEC_PUSH_N_U(V, O, N), 1)))

rklib_fun rk_forceinline void rki_vec_insert_arr_at(size_t elsize, void* restrict v, size_t i,
                                                    const void* restrict arr, size_t n) {
  size_t old_count = RKI_VEC_COUNT(v); // NOLINT(clang-analyzer-security.ArrayBound)
  char * src = (char*)v + rk_mult(i, elsize), *dst = src + rk_mult(n, elsize);
  memmove(dst, src, rk_mult(old_count - i, elsize));
  memcpy(src, arr, rk_mult(elsize, n));
  RKI_VEC_COUNT(v) = old_count + n; // NOLINT(clang-analyzer-security.ArrayBound)
}

#define RKI_VEC_INSERT_ARR_AT_U(V, I, O, N)                                                        \
  rki_vec_insert_arr_at(sizeof(*(V)), (V), rki_vec_assert_insertbounds(V, I), (O), (N))
#define RKI_VEC_INSERT_AT_U(V, I, O) RKI_VEC_INSERT_ARR_AT_U(V, I, ((typeof (*(V))[1]){(O)}), 1)

#define RKI_VEC_INSERT_AT(V, I, O)   (RKI_VEC_RESERVE_1(V), RKI_VEC_INSERT_AT_U(V, I, O))

#define RKI_VEC_INSERT_ARR_AT(V, I, O, N)                                                          \
  ((void)((N) && (RKI_VEC_RESERVE(V, vec_count(V) + (N)), RKI_VEC_INSERT_ARR_AT_U(V, I, O, N), 1)))

rklib_fun rk_forceinline void rki_vec_insert_at_unordered(size_t elsize, void* restrict vec,
                                                          size_t i, const void* restrict o) {
  size_t count = RKI_VEC_COUNT(vec); // NOLINT(clang-analyzer-security.ArrayBound)
  char*  dst   = (char*)vec + rk_mult(elsize, count);
  if (i < count) {
    char* src = (char*)vec + rk_mult(elsize, i);
    memcpy(dst, src, elsize);
    memcpy(src, o, elsize);
  } else {
    memcpy(dst, o, elsize);
  }
  ++RKI_VEC_COUNT(vec); // NOLINT(clang-analyzer-security.ArrayBound)
}

#define RKI_VEC_INSERT_AT_UNORDERED(V, I, O)                                                       \
  (RKI_VEC_RESERVE_1(V),                                                                           \
   rki_vec_insert_at_unordered(sizeof(*(V)), V, rki_vec_assert_insertbounds(V, I),                 \
                               ((typeof (*(V))[1]){(O)})))

rklib_fun rk_forceinline void rki_vec_erase_at_n(size_t elsize, void* v, size_t i, size_t n) {
  if (!n) { return; }
  char *dst = (char*)v + rk_mult(i, elsize), *src = dst + rk_mult(n, elsize);
  memmove(dst, src,
          rk_mult(((RKI_VEC_COUNT(v) -= n) - i),
                  elsize)); // NOLINT(clang-analyzer-security.ArrayBound)
}
#define RKI_VEC_ERASE_AT_N(V, I, N)                                                                \
  rki_vec_erase_at_n(sizeof(*(V)), (V), rki_vec_assert_erasebounds_n(V, I, N), (N))
#define RKI_VEC_ERASE_AT(V, I) RKI_VEC_ERASE_AT_N(V, I, 1)

#define RKI_VEC_ASSIGN(V, O, N)                                                                    \
  (RKI_VEC_RESERVE(V, N), (V) && (RKI_VEC_COUNT(V) = (N), rk_copy(V, O, N)))

#define RKI_VEC_FOREACH(vec, it)                                                                   \
  for (typeof(*(vec))*it = (vec), *const rki_var_end = vec_end(it); it != rki_var_end; ++it)

#define RKI_VEC_FOREACH_REVERSED(vec, it)                                                          \
  for (typeof(*(vec))*const rki_var_begin = (vec), *it = vec_end(rki_var_begin);                   \
       it != rki_var_begin && (--it, 1);)

#define RKI_VEC_ERASE_IF(vec, it, pred)                                                            \
  do {                                                                                             \
    RKI_IGNWARN_MSC_BEG(4114)                                                                      \
    typeof(vec) rki_var_vec = (vec);                                                               \
    if (!vec_count(rki_var_vec)) { break; }                                                        \
    typeof(*rki_var_vec)*rki_var_write = rki_var_vec, *const rki_var_end                           \
                                                      = rki_var_write                              \
                                                      + RKI_VEC_COUNT(rki_var_write);              \
    for (typeof(*rki_var_vec)* rki_var_it = rki_var_vec; rki_var_it != rki_var_end;                \
         ++rki_var_it) {                                                                           \
      typeof(*rki_var_vec)* const it = rki_var_it;                                                 \
      if (!(pred)) { *rki_var_write++ = *rki_var_it; }                                             \
    }                                                                                              \
    RKI_VEC_COUNT(rki_var_vec) = (size_t)(rki_var_write - rki_var_vec);                            \
    RKI_IGNWARN_MSC_END()                                                                          \
  } while (0)

// msvc sizeof returns 0
#define RKI_VEC_INIT_LIST_(T, arr, alloc)                                                          \
  memcpy(RKI_VEC_NEW_NONZERO(T, rk_COUNTOF(arr), rk_COUNTOF(arr), alloc), arr, sizeof(arr))
#define RKI_VEC_CONTRAV(T, x) _Generic(x, T: x, Allocator: (T){RKI_ZINIT})
#if RK_CUSTOM_ALLOCATORS
# define RKI_VEC_INIT_LIST(T, ...)                                                                 \
   _Generic(VA_FIRST(__VA_ARGS__),                                                                 \
       Allocator: RKI_VEC_INIT_LIST_(T, ((const T[]){VA_REST(__VA_ARGS__)}),                       \
                                     RKI_contrav(Allocator, VA_FIRST(__VA_ARGS__))),               \
       default: RKI_VEC_INIT_LIST_(                                                                \
                T, ((const T[]){RKI_VEC_CONTRAV(T, VA_FIRST(__VA_ARGS__)), VA_REST(__VA_ARGS__)}), \
                alloc_ctx))
#else
# define RKI_VEC_INIT_LIST(T, ...)                                                                 \
   RKI_VEC_INIT_LIST_(                                                                             \
       T, ((const T[]){RKI_VEC_CONTRAV(T, VA_FIRST(__VA_ARGS__)), VA_REST(__VA_ARGS__)}), 0)
#endif

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
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
