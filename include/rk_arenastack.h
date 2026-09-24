// SPDX-License-Identifier: MIT
/// @file rk_arenastack.h
/// @version 1.0
/// @defgroup rk_arenastack ArenaStack Allocator Interface
/// @brief Dynamic stack of arena allocators for growable, stack-like memory management with stable
/// allocation addresses.
///
/// The ArenaStack type manages a growable collection of fixed-size Arena allocators. It provides
/// convenience macros and functions for allocating, reallocating, clearing, and rewinding memory
/// from this stack of arenas.
///
/// Typical usage:
///   - Create an ArenaStack with arenastack_init(), specifying arena size and an optional
///     allocator.
///   - Allocate typed memory blocks using arenastack_new() or arenastack_new_aligned().
///   - Reclaim memory by rewinding with arenastack_rewind_to(), or by clearing all arenas via
///     `arenastack_clear()`.
///   - Release resources with arenastack_release().
///
/// Layout:
///   - Internally stores a Vec of Arena objects and a pointer to the currently active arena for
///     fast allocations.
///   - Designed for fast, stack-like allocation patterns with occasional rewinds and bulk clears.
///
/// Notes:
///   - Alignment arguments must be a power of two; undefined behaviour otherwise.
///   - Only one active ArenaStack instance is expected per intended allocation pool.
///
/// @see rk_alloc.h
/// @see rk_defs.h
/// @see rk_arena.h
/// @see rk_vec.h
/// @{

#ifndef RK_ARENASTACK_H
#define RK_ARENASTACK_H
#include "rk_arena.h"
#include "rk_vec.h"
RK_HEADER_BEGIN

/// @brief A dynamic stack of arenas used for memory allocation. Each arena is a fixed-size memory
/// block managed by the Arena allocator. The ArenaStack tracks a vec of arenas and the current
/// arena for allocation.
typedef struct ArenaStack {
  size_t     arena_size;
  Vec(Arena) arenas;
  size_t     cur;
} ArenaStack;

/// @brief `ArenaStack arenastack_init(size_t arena_size, Allocator alloc = alloc_ctx)` -
/// Initialises and returns a new ArenaStack with the desired capacity and allocator.
/// @param arena_size  The desired size of each arena
/// @param alloc       Optional allocator; defaults to `alloc_ctx`
/// @return A new ArenaStack
#define arenastack_init(arena_size, ...) rk_overload(RK__ARENASTACK_INIT, arena_size, ##__VA_ARGS__)

/// @brief Releases all arenas within the ArenaStack
static_fun void        arenastack_release(ArenaStack* self);

/// @brief Marks all Memory in the ArenaStack as reusable Clears all currently active arenas and
/// resets the current arena index. Memory in all arenas becomes available for reuse; arenas beyond
/// the current index are left unchanged until reused.
/// @return `self`, for chaining
static_fun ArenaStack* arenastack_clear(ArenaStack* self);

/// @brief Returns the current position of the active arena as an opaque marker. Pass to
/// `arenastack_rewind_to` to restore the ArenaStack to this state.
static_fun ArenaMark   arenastack_mark(const ArenaStack* self) {
  return self->arena_size ? arena_mark(&self->arenas[self->cur]) : (ArenaMark){rk_null};
}
/// @brief Rewinds the ArenaStack to a specific mark returned by `arenastack_mark()`, marking every
/// allocation in every Arena of the Stack as free until the mark is reached.
/// @return `self`, for chaining
static_fun ArenaStack* arenastack_rewind_to(ArenaStack* restrict self, ArenaMark mark);

/// @brief `void* arenastack_allocate(size_t nbytes, size_t align, ArenaStack* self)` - Allocates
/// `nbytes` bytes with the given alignment from the ArenaStack, growing into a new arena if
/// necessary. Prefer `arenastack_new` for typed allocations.
/// @param nbytes Number of bytes to allocate
/// @param align  Desired alignment; must be a power of two
/// @param self   ArenaStack to allocate from
/// @return Pointer to the allocated memory
static_fun void*       arenastack_allocate(size_t nbytes, size_t align, ArenaStack* self);

/// @brief `T* arenastack_new(T, size_t count, ArenaStack* arena_stack)` - Create a new allocation
/// in the arena for a given type T and count.
/// @param  T           The type to allocate
/// @param  count       Number of elements of type T to allocate
/// @param  arena_stack Pointer to the ArenaStack to allocate from
/// @return Pointer to the allocated memory
#define arenastack_new(T, count, arena_stack) RK__arenastack_NEW(T, count, arena_stack)

/// @brief `T* arenastack_new_aligned(T, size_t count, size_t align, ArenaStack* arena_stack)` -
/// Create a new, allocation in the ArenaStack for a given type T and count with a given alignment.
/// @param T           The type to allocate
/// @param count       Number of elements of type T to allocate
/// @param align       The desired alignment (must be a power of two)
/// @param arena_stack Pointer to the ArenaStack to allocate from
/// @return Pointer to the allocated memory.
/// @note If alignment is not a power of two, behaviour is undefined
#define arenastack_new_aligned(T, count, align, arena_stack)                                       \
  RK__arenastack_ALIGNED_NEW(T, count, align, arena_stack)

static_fun alloc_allocation_f   RK__arenastack_allocate;
static_fun alloc_reallocation_f RK__arenastack_reallocate;
static_fun alloc_deallocation_f RK__arenastack_deallocate;
static const AllocatorVTable arenastack_allocator_vtable = {.alloc_f   = RK__arenastack_allocate,
                                                            .realloc_f = RK__arenastack_reallocate,
                                                            .dealloc_f = RK__arenastack_deallocate};

static_fun Allocator         arenastack_to_alloc(ArenaStack* self) {
  return (Allocator){.vtab = &arenastack_allocator_vtable, .ctx = self};
}

static_fun Allocator arenastack_allocator(const ArenaStack* self) {
  return vec_allocator(self->arenas);
}

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RK__arenastack_ALIGNED_NEW(T, count, align, arena_stack)                                   \
  ((typeof(T)*)(alloc_log_new, rk_assert_valid_align(T, align),                                    \
                arenastack_allocate(sizeof_n(T, count), align, arena_stack)))

#define RK__arenastack_NEW(T, count, arena_stack)                                                  \
  ((typeof(T)*)(alloc_log_new, arenastack_allocate(sizeof_n(T, count), alignof(T), arena_stack)))

static_fun void arenastack_release(ArenaStack* self) {
  RK_IFALLOC(Allocator alloc = vec_allocator(self->arenas);)
  vec_foreach(self->arenas, arena) {
    alloc_deallocate(arena->beg, (size_t)(arena->end - arena->beg), align_max RK_IFALLOC(, alloc));
  }
  vec_release(self->arenas);
  self->arena_size = 0, self->cur = 0;
}

static_fun ArenaStack* arenastack_clear(ArenaStack* self) {
  if rk_unlikely (!self->arena_size) { return self; }
  for (size_t cur = self->cur, i = 0; i <= cur; ++i) { arena_clear(&self->arenas[i]); }
  return self->cur = 0, self;
}

static_fun ArenaStack* arenastack_rewind_to(ArenaStack* restrict self, ArenaMark mark) {
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
  rk_assert(0 && "Pointer was not allocated by this stack"), unreachable();
}

static_fun ArenaStack RK__arenastack_init(size_t cap RK_IFALLOC(, Allocator alloc)) {
  rk_assert_allocator_valid(alloc);
  cap = stdc_bit_ceil(cap); /*1 if cap==0*/
  return (ArenaStack){
      .arena_size = cap,
      .arenas     = vec_init_list(
          Arena, RK_IFALLOC(alloc, ) arena_init(
                     (unsigned char*)alloc_allocate(cap, align_max RK_IFALLOC(, alloc)), cap)),
      .cur = 0};
}
#define RK__ARENASTACK_INIT(_cap, _alloc)  RK__arenastack_init(_cap RK_IFALLOC(, _alloc))
#define RK__ARENASTACK_INIT2(_cap, _alloc) rk_disable_if(RK__ARENASTACK_INIT(_cap, _alloc))
#define RK__ARENASTACK_INIT1(cap)          RK__ARENASTACK_INIT(cap, alloc_ctx)

#define RK__arena_alloc_init(_SIZE, _ALIGN, _ALLOC)                                                \
  arena_init((unsigned char*)alloc_allocate(_SIZE, _ALIGN RK_IFALLOC(, _ALLOC)), _SIZE)

static_fun rk_alloc_alignsize(2, 1) void* RK__arenastack_allocate(size_t nbytes, size_t align,
                                                                  void* ctx) {
  rk_assert_align_pow2(align);
  ArenaStack* self   = (ArenaStack*)ctx;
  size_t      needed = nbytes + (align - 1);
  if (!self->arena_size) { *self = arenastack_init(stdc_bit_ceil(needed)); }
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
  // The backing chunk only ever needs align_max: `needed` above already
  // reserves nbytes + (align - 1) slack, so arena_try_allocate() can carve
  // out an `align`-aligned pointer from any align_max-aligned chunk.
  // Matches the align_max used to deallocate arenas in arenastack_release().
  vec_insert_at_unordered(self->arenas, self->cur,
                          RK__arena_alloc_init(stdc_bit_ceil(rk_MAX(needed, self->arena_size)),
                                               align_max, vec_allocator(self->arenas)));
  return arena_allocate(nbytes, align, &self->arenas[self->cur]);
}

static_fun void RK__arenastack_deallocate(void* ptr, size_t old_size, size_t align, void* ctx) {
  ArenaStack* self = (ArenaStack*)ctx;
  RK__arena_deallocate(ptr, old_size, align, &self->arenas[self->cur]);
}

static_fun rk_alloc_alignsize(4, 3) void* RK__arenastack_reallocate(void* ptr, size_t old_size,
                                                                    size_t new_size, size_t align,
                                                                    void* ctx) {
  rk_assert_align_pow2(align);
  ArenaStack* self = (ArenaStack*)ctx;
  if (!old_size) { return arenastack_allocate(new_size, align, self); }
  if ((arena_is_top_allocation(&self->arenas[self->cur], ptr, old_size)
       && arena_try_resize_top(old_size, new_size, &self->arenas[self->cur]))
      || new_size <= old_size) {
    return ptr;
  }
  void* res = arenastack_allocate(new_size, align, self);
  rk_memcpy(res, ptr, rk_MIN(old_size, new_size));
  return res;
}

static_fun rk_alloc_alignsize(2, 1) void* arenastack_allocate(size_t nbytes, size_t align,
                                                              ArenaStack* self) {
  return RK__arenastack_allocate(nbytes, align, self);
}

#undef RK__arena_alloc_init

/// @endcond

RK_HEADER_END
/// @}
#endif // RK_ARENASTACK_H

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
