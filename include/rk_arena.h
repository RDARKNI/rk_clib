/// @file rk_arena.h
/// @version 2.0
/// @defgroup rk_arena Arena Allocator Interface
/// @brief Arena Allocator Implementation
///
/// Provides a simple linear/stack-style arena allocator for fast temporary memory allocation.
/// Supports reset and simple realloc-like behavior. Also provides integration with the Allocator
/// interface defined in `rk_alloc.h` for use as a plug-in allocator.
///
/// Arena allocators are efficient when many small allocations are needed with the same lifetime, as
/// freeing all allocations at once is trivial.
///
/// @note This arena is not thread-safe, it is recommended to create several thread-local arenas
/// instead
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{
#ifndef RK_ARENA_H
#define RK_ARENA_H
#include "rk_alloc.h"
RK_HEADER_BEGIN
RK__IGNWARN_CLANG_BEG("-Wreturn-type-c-linkage")

/// @brief Linear / stack allocator for fast, temporary memory management.
/// @details The Arena allocator manages a region of memory where allocations increment a pointer,
/// allowing fast allocation and reset.
typedef struct Arena {
  unsigned char* beg; ///< Start of arena memory block
  unsigned char* cur; ///< Current Position in arena memory block
  unsigned char* end; ///< End of arena memory block
} Arena;

/// @brief `Arena arena_init_static(unsigned char arr[])` Initialises an Arena from an array as
/// storage at compile time. This macro allows for non-dynamically allocated memory to use the arena
/// allocator and the more general allocator interface.
/// @param array_non_compound_literal The byte array to serve as the arena's backing memory
/// @return New Arena with the array as backing storage
/// @warning Do not use with compound literals; use `arena_init()` instead.
#define arena_init_static(array_non_compound_literal)                                              \
  RK__arena_init_static(array_non_compound_literal)

/// @brief Initialises an Arena from an array as storage at runtime. Allows for any memory to use
/// the arena allocator and the more general allocator interface.
/// @param arr The byte array to serve as the arena's backing memory
/// @param len The length of `arr`, in bytes
/// @return New Arena using the array as backing storage
static_fun rk_pure Arena arena_init(unsigned char* arr, size_t len) {
  return (Arena){.beg = arr, .cur = arr, .end = arr ? arr + len : 0};
}

/// @brief Returns the number of bytes an Arena can allocate in total.
static_fun rk_pure size_t arena_cap(const Arena* self) {
  return rk_likely(self) ? (size_t)(self->end - self->beg) : 0;
}

/// @brief Returns the number of bytes an Arena has allocated.
static_fun rk_pure size_t arena_used(const Arena* self) {
  return rk_likely(self) ? (size_t)(self->cur - self->beg) : 0;
}

/// @brief Returns the number of bytes an Arena can allocate.
static_fun rk_pure size_t arena_remaining(const Arena* self) {
  return rk_likely(self) ? (size_t)(self->end - self->cur) : 0;
}

/// @brief Returns whether the arena has not allocated any memory.
static_fun rk_pure bool  arena_is_empty(const Arena* self) { return self->cur == self->beg; }

/// @brief Resets the arena, marking all of its allocations as free.
/// @return `self`, for chaining
static_fun Arena*        arena_clear(Arena* self) { return self->cur = self->beg, self; }

typedef struct ArenaMark ArenaMark;

/// @brief Returns the current position of the arena as an opaque marker. Pass to `arena_rewind_to`
/// to restore the arena to this state.
/// @return Pointer to the current position in the arena
static_fun ArenaMark     arena_mark(const Arena* self);

/// @brief Rewinds the arena's current pointer to `mark`, marking memory starting from `mark` as
/// free.
/// @return `self`, for chaining
/// @attention Behavior is undefined if `mark` was not allocated by the arena.
static_fun Arena*        arena_rewind_to(Arena* self, ArenaMark mark);

/// @brief Returns whether `ptr` is the most recently made allocation of the given `size`, i.e.
/// whether it ends exactly at the arena's current position.
/// @param ptr The allocation to check. Must be an allocation made by the arena.
/// @param size Size of the allocation in bytes
/// @return `true` if `ptr` is the top allocation, `false` otherwise
static_fun bool          arena_is_top_allocation(const Arena* self, const void* ptr, size_t size) {
  return (const unsigned char*)ptr + size == self->cur;
}

/// @brief `void* arena_allocate(size_t nbytes, size_t align, Arena* self)` - Allocates `nbytes`
/// bytes with the given alignment. Aborts on failure via `RK_ARENA_FAIL`. Prefer `arena_new` for
/// typed allocations.
/// @param nbytes Number of bytes to allocate
/// @param align  Desired alignment; must be a power of two
/// @param self   Pointer to the arena to allocate from
/// @return Pointer to the allocated memory
static_fun void* arena_allocate(size_t nbytes, size_t align, Arena* self);

/// @brief `void* arena_try_allocate(size_t nbytes, size_t align, Arena* self)`
/// - like `arena_allocate()` but returns NULL if the arena does not have enough space instead of
///   invoking the failure handler.
static_fun void* arena_try_allocate(size_t nbytes, size_t align, Arena* self);

/// @brief `void* arena_resize_top(size_t old_size, size_t new_size, Arena* self)` - Resizes the
/// most recent allocation in the arena by moving the cursor. Aborts on failure via `RK_ARENA_FAIL`.
/// Prefer `arena_extend` for typed resizes.
/// @param old_size Current size of the allocation in bytes
/// @param new_size Desired size of the allocation in bytes
/// @param self     Pointer to the arena owning the allocation
/// @return `ptr` on success
static_fun void* arena_resize_top(size_t old_size, size_t new_size, Arena* self);

/// @brief `void* arena_try_resize_top(size_t old_size, size_t new_size, Arena* self)` - Like
/// `arena_resize_top()` but returns NULL if the arena does not have enough space instead of
/// invoking the failure handler.
static_fun void* arena_try_resize_top(size_t old_size, size_t new_size, Arena* self);

/// @brief `T* arena_new(T, size_t count, Arena* arena)` - Creates a new allocation in the arena for
/// a given type and count.
/// @param  T     The type to allocate
/// @param  count Number of elements of type T to allocate
/// @param  arena Pointer to the arena to allocate from
/// @return T* Pointer to the allocated memory
#define arena_new(T, count, arena)                RK__arena_NEW(T, count, arena)

/// @brief `T* arena_new_aligned(T, size_t count, size_t alignment, Arena* arena)` - Creates a new
/// allocation in the arena for a given type T and count with a given alignment independent of type.
/// @param T      The type to allocate
/// @param count  Number of elements of type T to allocate
/// @param align  Desired alignment of the allocation
/// @param arena  Pointer to the arena to allocate from
/// @return Pointer to the allocated memory.
/// @note Alignment must be a power of two.
#define arena_new_aligned(T, count, align, arena) RK__arena_ALIGNED_NEW(T, count, align, arena)

/// @brief `T* arena_extend(T* ptr, size_t old_count, size_t new_count, Arena* arena)` - Resizes the
/// most recent allocation from `old_count` to `new_count` elements. Aborts on failure via
/// `RK_ARENA_FAIL`.
/// @param ptr Pointer to the allocation to extend; must be the most recent allocation in the arena
/// @param old_count Current number of allocated elements
/// @param new_count Desired number of elements after resizing
/// @param arena     Arena owning the allocation
/// @return `ptr` on success, cast to the same pointer type
#define arena_extend(ptr, old_count, new_count, arena)                                             \
  ((typeof(ptr))RK__arena_extend(ptr, sizeof_n(*(ptr), old_count), sizeof_n(*(ptr), new_count),    \
                                 arena))

/// @brief `T* arena_try_new(T, size_t count, Arena* arena)` - Like `arena_new()`, but returns
/// `NULL` if the arena does not have enough space instead of invoking the failure handler.
#define arena_try_new(T, count, arena) arena_try_new_aligned(T, count, alignof(T), arena)

/// @brief `T* arena_try_new_aligned(T, size_t count, size_t align, Arena* arena)` like
/// `arena_new_aligned()`, but returns `NULL` if the arena does not have enough space instead of
/// invoking the failure handler.
#define arena_try_new_aligned(T, count, align, arena)                                              \
  (rk_assert_valid_align(T, align), (T*)arena_try_allocate(sizeof_n(T, count), align, arena))

/// @brief `T* arena_try_extend(T* ptr, size_t old_count, size_t new_count, Arena* arena)` - Like
/// `arena_extend()` but returns `NULL` if the arena does not have enough space instead of invoking
/// the failure handler.
#define arena_try_extend(ptr, old_count, new_count, arena)                                         \
  ((typeof(ptr))RK__arena_try_extend(ptr, sizeof_n(*(ptr), old_count),                             \
                                     sizeof_n(*(ptr), new_count), arena))

static_fun alloc_allocation_f   RK__arena_allocate;
static_fun alloc_reallocation_f RK__arena_reallocate;
static_fun alloc_deallocation_f RK__arena_deallocate;
static const AllocatorVTable    arena_allocator_vtable = {.alloc_f   = RK__arena_allocate,
                                                          .realloc_f = RK__arena_reallocate,
                                                          .dealloc_f = RK__arena_deallocate};

/// @brief `Allocator arena_to_alloc_static(Arena* arena)` - Creates an Allocator from an Arena
/// allowing it to serve as backing allocator for other rk_clib types. Works at compile-time and can
/// be used for static initialisation.
#define arena_to_alloc_static(arena) {.vtab = &arena_allocator_vtable, .ctx = (arena)}

/// @brief Creates an Allocator from an Arena at runtime, allowing it to serve as backing allocator
/// for other rk_clib types.
static_fun Allocator arena_to_alloc(Arena* arena) {
  return (Allocator)arena_to_alloc_static(arena);
}

/// @brief Type of an Allocator object managing an array of size `size` using an Arena to manage its
/// memory.
#define arr_allocator(size)                                                                        \
  struct {                                                                                         \
    union {                                                                                        \
      const Allocator alloc;                                                                       \
      const struct {                                                                               \
        const AllocatorVTable* vtab;                                                               \
        void*                  ctx;                                                                \
      };                                                                                           \
    };                                                                                             \
    Arena                     arena;                                                               \
    alignas_max unsigned char arr[(size)];                                                         \
  }

/// @brief Initialise an arr_allocator object by passing its address.
#define arr_allocator_init(self)                                                                   \
  {.vtab = &arena_allocator_vtable, .ctx = &(self)->arena, .arena = arena_init_static((self)->arr)}

/// @brief Declares and initializes a static, stack-allocated Allocator instance with internal
/// arena-based storage.
/// @details This macro creates a compound struct on the stack (or in static scope) that embeds:
/// - a fixed-size memory buffer (`arr[size]`) as backing storage,
/// - and an `Arena` allocator using that buffer.
/// - an `Allocator` interface (via union for compatibility), The resulting variable can be treated
///   like an `Allocator` and used anywhere the `rk_clib` allocator interface is expected. This is
///   particularly useful for creating fast, temporary allocators with automatic storage duration.
/// @param name The name of the variable to declare
/// @param size The size (in bytes) of the internal backing memory buffer, must be a compile-time
/// constant.
///
/// Usage:
/// ```c
///     arr_allocator_create(temp_alloc, 4096);
///     int* ptr = alloc_new(int, 10, &temp_alloc);
/// ```
#define arr_allocator_create(name, size) arr_allocator(size) name = arr_allocator_init(&name)

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

typedef struct ArenaMark { unsigned char* pos; } ArenaMark;

static_fun ArenaMark arena_mark(const Arena* self) { return (ArenaMark){.pos = self->cur}; }

#define rk_assert_ptr_in_arena(arena, ptr)                                                         \
  rk_assert(rk_ptr_in_range(ptr, (arena)->beg, (arena)->cur)                                       \
            && "Pointer was not allocated by this Arena")

static_fun Arena* arena_rewind_to(Arena* self, ArenaMark mark) {
  if (mark.pos == self->cur) { return self; }
  rk_assert_ptr_in_arena(self, mark.pos);
  self->cur = mark.pos;
  return self;
}

static_fun rk_alloc_alignsize(2, 1) void* arena_try_allocate(size_t nbytes, size_t align,
                                                             Arena* self) {
  if rk_unlikely (!self->cur) { return rk_null; }
  size_t pad = rk_align_pad(self->cur, align), avail = arena_remaining(self);
  if (avail < pad || avail - pad < nbytes) { return rk_null; }
  unsigned char* ptr = self->cur + pad;
  self->cur          = ptr + nbytes;
  return ptr;
}

static_fun rk_alloc_size(2) void* arena_try_resize_top(size_t old_size, size_t new_size,
                                                       Arena* self) {
  if (arena_remaining(self) + old_size < new_size) { return rk_null; }
  unsigned char* ptr = self->cur - old_size;
  self->cur          = ptr + new_size;
  return ptr;
}

static_fun rk_alloc_size(2) void* arena_resize_top(size_t old_size, size_t new_size, Arena* self) {
  void* res = arena_try_resize_top(old_size, new_size, self);
  RK_ARENA_FAIL(res, self, (self->cur - old_size), align_max, new_size);
  return res;
}

static_fun rk_alloc_alignsize(2, 1) void* RK__arena_allocate(size_t nbytes, size_t align,
                                                             void* ctx) {
  void* ptr = arena_try_allocate(nbytes, align, (Arena*)ctx);
  RK_ARENA_FAIL(ptr, (Arena*)ctx, rk_null, align, nbytes);
  return ptr;
}

static_fun void RK__arena_deallocate(void* ptr, size_t old_size, size_t align rk_unused,
                                     void* ctx) {
  if (arena_is_top_allocation((Arena*)ctx, ptr, old_size)) {
    (void)arena_try_resize_top(old_size, 0, (Arena*)ctx);
  }
}
static_fun rk_alloc_alignsize(4, 3) void* RK__arena_reallocate(void* ptr, size_t old_size,
                                                               size_t new_size, size_t align,
                                                               void* ctx) {
  rk_assert_align_pow2(align);
  Arena* self = (Arena*)ctx;
  if (!old_size) { return RK__arena_allocate(new_size, align, self); }
  if ((arena_is_top_allocation(self, ptr, old_size)
       && arena_try_resize_top(old_size, new_size, self))
      || new_size <= old_size) { // non-top shrinks are no-ops
    return ptr;
  }
  void* res = RK__arena_allocate(new_size, align, ctx);
  rk_memcpy(res, ptr, rk_min(old_size, new_size));
  return res;
}

static_fun rk_alloc_alignsize(2, 1) void* arena_allocate(size_t nbytes, size_t align, Arena* self) {
  return RK__arena_allocate(nbytes, align, self);
}
static_fun rk_alloc_size(3) void* RK__arena_try_extend(void* ptr, size_t old_size, size_t new_size,
                                                       Arena* self) {
  rk_assert(arena_is_top_allocation(self, ptr, old_size)
            && "Can only resize the top allocation of the arena");
  return arena_try_resize_top(old_size, new_size, self);
}

static_fun rk_alloc_size(3) void* RK__arena_extend(void* ptr, size_t old_size, size_t new_size,
                                                   Arena* self) {
  void* r = RK__arena_try_extend(ptr, old_size, new_size, self);
  RK_ARENA_FAIL(r, self, (self->cur - old_size), align_max, new_size);
  return r;
}

#define RK__arena_init_static(arr)                                                                 \
  {.beg = (arr) + rk_ensure_valid_storage_type(arr), .cur = (arr), .end = (arr) + sizeof(arr)}

#define RK__arena_ALIGNED_NEW(T, count, align, arena)                                              \
  ((typeof(T)*)(alloc_log_new, rk_assert_valid_align(T, align),                                    \
                arena_allocate(sizeof_n(T, count), align, arena)))
#define RK__arena_NEW(T, count, arena)                                                             \
  ((typeof(T)*)(alloc_log_new, arena_allocate(sizeof_n(T, count), alignof(T), arena)))

/// @endcond
RK__IGNWARN_CLANG_END()
RK_HEADER_END
/// @}
#endif // RK_ARENA_H
