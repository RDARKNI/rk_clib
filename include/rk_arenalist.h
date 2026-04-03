/// @file rk_arenalist.h
/// @version 1.0
/// @defgroup rk_arenalist Arenalist Allocator Interface
/// @brief Dynamic list of arena allocators for pooled memory management.
///
/// The ArenaList type manages a growable collection of fixed-size Arena
/// allocators. It provides convenience macros and functions for allocating,
/// reallocating, clearing, and rewinding memory from this list of arenas.
///
/// Typical usage:
///   - Create an ArenaList with arenalist_init(), specifying arena size and
///     an optional allocator.
///   - Allocate typed memory blocks using arenalist_new() or
///     arenalist_new_aligned().
///   - Reclaim memory by rewinding with arenalist_rewind_to(), or by clearing
///   all arenas via `arenalist_clear()`.
///   - Release resources with arenalist_release().
///
/// Layout:
///   - Internally stores a Vec of Arena objects and a pointer to the
///     currently active arena for fast allocations.
///   - Designed for fast, stack-like allocation patterns with occasional
///     rewinds and bulk clears.
///
/// Notes:
///   - Alignment arguments must be a power of two; undefined behaviour
///     otherwise.
///   - Only one active ArenaList instance is expected per intended allocation
///     pool.
///
/// @see rk_alloc.h
/// @see rk_defs.h
/// @see rk_arena.h
/// @see rk_vec.h
/// @{

#ifndef RK_ARENALIST_H
#define RK_ARENALIST_H
#include "rk_arena.h"
#include "rk_vec.h"
RK_HEADER_BEGIN

/// @brief A dynamic list of arenas used for memory allocation.
/// Each arena is a fixed-size memory block managed by the Arena allocator.
/// The ArenaList tracks a vec of arenas and the current arena for
/// allocation.
typedef struct ArenaList {
  size_t     arena_size;
  Vec(Arena) arenas;
  size_t     cur;
} ArenaList;

/// @brief `ArenaList arenalist_init(size_t arena_size, Allocator alloc =
/// alloc_ctx)` - Initialises and returns a new arenalist with the desired
/// capacity and allocator.
/// @param arena_size  The desired size of each arena
/// @param alloc       Allocator Optional parameter - The allocator
/// @return A new ArenaList
#define arenalist_init(arena_size, ...)                                        \
  rk_overload(RK__ARENALIST_INIT, arena_size, ##__VA_ARGS__)

/// @brief Releases all arenas within the ArenaList
static_fun void       arenalist_release(ArenaList* alist);

/// @brief Clears all arenas in the ArenaList, marking all the memory as free.
/// @return `self`, for chaining
static_fun ArenaList* arenalist_clear(ArenaList* self);

/// @brief Returns the current position of the active arena as an opaque
///        marker. Pass to `arenalist_rewind_to` to restore the ArenaList to
///        this state.
static_fun ArenaMark  arenalist_mark(const ArenaList* self) {
  return self->arena_size ? arena_mark(&self->arenas[self->cur])
                           : (ArenaMark){rk_null};
}
/// @brief Rewinds the ArenaList to a specific mark returned by
/// `arenalist_mark()`, marking every allocation in every Arena of the List as
/// free until the mark is reached.
/// @return `self`, for chaining
static_fun ArenaList* arenalist_rewind_to(ArenaList* restrict self,
                                          ArenaMark mark);

/// @brief `void* arenalist_allocate(size_t nbytes, size_t align, ArenaList*
/// self)` - Allocates `nbytes` bytes with the given alignment from the
/// ArenaList, growing into a new arena if necessary. Prefer `arenalist_new`
/// for typed allocations.
/// @param nbytes Number of bytes to allocate
/// @param align  Desired alignment; must be a power of two
/// @param self   ArenaList to allocate from
/// @return Pointer to the allocated memory
static_fun void*      arenalist_allocate(size_t nbytes, size_t align,
                                         ArenaList* self);

/// @brief `T* arenalist_new(T, size_t count, ArenaList* alist)` -
/// Create a new allocation in the arena for a given type T and count.
/// @param  T     The type to allocate
/// @param  count Number of elements of type T to allocate
/// @param  alist ArenaList* The arenalist to allocate from
/// @return Pointer to the allocated memory
#define arenalist_new(T, count, alist) RK__arenalist_new(T, count, alist)

/// @brief `T* arenalist_new_aligned(T, size_t count, size_t align, ArenaList*
/// alist)` - Create a new, allocation in the arena for a given type T and count
/// with a given alignment.
/// @param T         The type to allocate
/// @param count     Number of elements of type T to allocate
/// @param align     The desired alignment (must be a power of two)
/// @param alist     ArenaList* The arena to allocate from
/// @return Pointer to the allocated memory.
/// @note If alignment is not a power of two, behaviour is undefined
#define arenalist_new_aligned(T, count, align, alist)                          \
  RK__arenalist_new_aligned(T, count, align, alist)

static_fun alloc_allocation_f   RK__arenalist_allocate;
static_fun alloc_reallocation_f RK__arenalist_reallocate;
static_fun alloc_deallocation_f RK__arenalist_deallocate;
static const AllocatorVTable    arenalist_allocator_vtable
    = {.alloc_f   = RK__arenalist_allocate,
       .realloc_f = RK__arenalist_reallocate,
       .dealloc_f = RK__arenalist_deallocate};
static_fun Allocator arenalist_to_alloc(ArenaList* self) {
  return (Allocator){.vtab = &arenalist_allocator_vtable, .ctx = self};
}

////////////////////////////////////////////////////////////////////////////////
/////////////////////////Implementation Details/////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RK__arenalist_new_aligned(T, count, align, alist)                      \
  ((typeof(T)*)(alloc_log_new, rk_assert_valid_align(T, align),                \
                RK__arenalist_allocate(sizeof_n(T, count), align, alist)))

#define RK__arenalist_new(T, count, alist)                                     \
  RK__arenalist_new_aligned(T, count, alignof(T), alist)

static_fun void arenalist_release(ArenaList* self) {
  Allocator alloc = vec_allocator(self->arenas);
  vec_foreach(self->arenas, arena) {
    alloc_deallocate(arena->beg, arena->end - arena->beg, 1, alloc);
  }
  vec_release(self->arenas);
  self->arena_size = 0, self->cur = 0;
}

static_fun ArenaList* arenalist_clear(ArenaList* self) {
  if rk_unlikely (!self->arena_size) { return self; }
  for (size_t cur = self->cur, i = 0; i <= cur; ++i) {
    arena_clear(&self->arenas[i]);
  }
  return self->cur = 0, self;
}

static_fun ArenaList* arenalist_rewind_to(ArenaList* restrict self,
                                          ArenaMark mark) {
  const unsigned char* ptr = mark.pos;
  if rk_unlikely (!ptr) { return self; }
  for (size_t i = self->cur + 1; i-- > 0;) {
    Arena* arena = &self->arenas[i];
    if (ptr == arena->cur || rk_ptr_in_range(ptr, arena->beg, arena->cur)) {
      arena_rewind_to(arena, mark);
      self->cur = i - (i > 0 && arena_is_empty(arena));
      return self;
    }
    arena_clear(arena);
  }
  rk_assert(!"Pointer was not allocated by this list"), unreachable();
}

static_fun ArenaList
    RK__arenalist_init(size_t cap RK_IFALLOC(, Allocator alloc)) {
  rk_assert_allocator_valid(alloc);
  cap = stdc_bit_ceil(cap); /*1 if cap==0*/
  return (ArenaList){
      .arena_size = cap,
      .arenas     = vec_init_list(
          Arena,
          RK_IFALLOC(alloc, ) arena_init(
              (unsigned char*)alloc_allocate(cap, align_max, alloc), cap)),
      .cur = 0};
}

#define RK__ARENALIST_INIT2(cap, allocator)                                    \
  RK__arenalist_init(cap RK_IFALLOC(, allocator))
#define RK__ARENALIST_INIT1(cap) RK__ARENALIST_INIT2(cap, alloc_ctx)

#define arena_alloc_init(_SIZE, _ALIGN, _ALLOC)                                \
  arena_init((unsigned char*)alloc_allocate(_SIZE, _ALIGN, _ALLOC), _SIZE)

static_fun rk_alloc_alignsize(2, 1) void* RK__arenalist_allocate(size_t nbytes,
                                                                 size_t align,
                                                                 void*  _self) {
  rk_assert_align_pow2(align);
  ArenaList* self   = (ArenaList*)_self;
  size_t     needed = nbytes + (align - 1);
  if (!self->arena_size) { *self = arenalist_init(stdc_bit_ceil(needed)); }
  void* res = arena_try_allocate(nbytes, align, &self->arenas[self->cur]);
  if (res) { return res; }
  ++self->cur; // slow path: Check if there's a large-enough preallocated arena
  for (size_t i = self->cur, count = vec_count(self->arenas); i < count; ++i) {
    if (needed <= arena_cap(&self->arenas[i])) {
      if (i != self->cur) { rk_SWAP(self->arenas[i], self->arenas[self->cur]); }
      arena_clear(&self->arenas[self->cur]);
      return arena_allocate(nbytes, align, &self->arenas[self->cur]);
    }
  }
  // no large-enough arena found; create new one
  size_t cap = stdc_bit_ceil(rk_MAX(needed, self->arena_size));
  vec_insert_at_unordered(self->arenas, self->cur,
                          arena_alloc_init(cap, rk_MAX(align_max, align),
                                           vec_allocator(self->arenas)));
  return arena_allocate(nbytes, align, &self->arenas[self->cur]);
}

static_fun void RK__arenalist_deallocate(void* ptr, size_t old_size,
                                         size_t align, void* _self) {
  ArenaList* self = (ArenaList*)_self;
  RK__arena_deallocate(ptr, old_size, align, &self->arenas[self->cur]);
}

static_fun rk_alloc_alignsize(4, 3) void* RK__arenalist_reallocate(
    void* ptr, size_t old_size, size_t new_size, size_t align, void* _self) {
  rk_assert_align_pow2(align);
  ArenaList* self = (ArenaList*)_self;
  if (!old_size) { return arenalist_allocate(new_size, align, self); }
  if ((arena_is_top_allocation(&self->arenas[self->cur], ptr, old_size)
       && arena_try_resize_top(old_size, new_size, &self->arenas[self->cur]))
      || new_size <= old_size) {
    return ptr;
  }
  void* res = arenalist_allocate(new_size, align, self);
  rk_memcpy(res, ptr, rk_MIN(old_size, new_size));
  return res;
}

static_fun rk_alloc_alignsize(2, 1) void* arenalist_allocate(size_t     nbytes,
                                                             size_t     align,
                                                             ArenaList* self) {
  return RK__arenalist_allocate(nbytes, align, self);
}

#undef arena_alloc_init

/// @endcond

RK_HEADER_END
/// @}
#endif /* RK_ARENALIST_H */
