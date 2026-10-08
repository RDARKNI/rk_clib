// SPDX-License-Identifier: MIT
/// @file rk_arenastack.h
/// @version 1.0.0
/// @defgroup rk_arenastack ArenaStack Allocator Interface
/// @brief Growable collection of arenas for temporary allocations and bulk reuse.
/// @details Stores a Vec of Arena objects and the index of the active arena. Growth allocates or
/// reuses backing buffers without moving existing allocations; reallocation may move an allocation.
/// Initialize with arenastack_init(), allocate with arenastack_allocate() or the typed macros,
/// reclaim storage with arenastack_clear() or arenastack_rewind_to(), and release it with
/// arenastack_release().
/// @note Concurrent access involving allocation, resizing, clearing, rewinding, or release requires
/// external synchronization. Independent stacks may be used independently, subject to their
/// allocator's thread-safety requirements.
/// @see rk_alloc.h
/// @see rk_arena.h
/// @see rk_vec.h
/// @{
#ifndef RK_ARENASTACK_H
#define RK_ARENASTACK_H
#include "rk_arena.h"
#include "rk_vec.h"
RKI_HEADER_BEGIN

/// @brief Owns arena backing buffers and a vector of arena descriptors, with an active arena index.
/// @note ArenaStack pointer arguments must be non-null and refer to initialized objects. A
/// zero-initialized stack supports lazy allocation and has no backing buffers initially.
/// Copying this structure does not duplicate its owned resources; do not treat copies as
/// independent owners. Allocator handles borrowing the stack require it to remain at the same
/// address.
typedef struct ArenaStack {
  size_t     arena_size;
  Vec(Arena) arenas;
  size_t     cur;
} ArenaStack;

/// @brief `ArenaStack arenastack_init(size_t arena_size, Allocator alloc = alloc_ctx)` -
/// Initializes an owning stack and allocates its first arena.
/// @param arena_size Minimum backing-buffer size for new arenas, rounded up to a power of two; zero
/// selects one byte. The rounded size must be representable in size_t.
/// @param alloc Backing allocator; defaults to alloc_ctx. Explicit selection requires custom
/// allocators.
/// @return An initialized ArenaStack owning its first buffer and descriptor vector.
/// @note Larger requests can create larger arenas; this is not a fixed size for every arena.
/// Allocation failures follow the backing allocator's failure policy.
#define arenastack_init(arena_size, ...)                                                           \
  RKI_OVERLOAD(RKI_ARENASTACK_INIT, arena_size, ##__VA_ARGS__)

/// @brief Releases every arena backing buffer and the descriptor vector, resetting the stack.
/// @note Invalidates all allocations and marks. A zero-initialized stack is supported. A subsequent
/// allocation lazily initializes the stack using alloc_ctx; the previous allocator is not retained.
rklib_fun void              arenastack_release(ArenaStack* self);

/// @brief Returns the backing allocator associated with the arena descriptor vector.
/// @note Returns alloc_ctx for a zero-initialized stack or when custom allocators are disabled.
/// This is the allocator used to obtain buffers, not an Allocator adapter allocating within this
/// stack.
/// @see arenastack_to_alloc
rklib_fun rk_pure Allocator arenastack_allocator(const ArenaStack* self) {
  return vec_allocator(self->arenas);
}

/// @brief Reclaims all allocations for reuse without releasing backing storage.
/// @return self, for chaining.
/// @note Clears arenas through the active index and resets that index to zero. Later arenas are
/// retained and cleared when reused. Invalidates previous allocations and marks; does not clear
/// their bytes. A zero-initialized stack is supported.
rklib_fun ArenaStack*       arenastack_clear(ArenaStack* self);

/// @brief Saves the active arena's cursor for arenastack_rewind_to().
/// @return A marker for the current position, or a null-position marker for a zero-initialized
/// stack.
/// @note Does not allocate or keep allocations alive. A null-position mark denotes the stack's
/// starting position: rewinding to it clears the stack, even if it has since been lazily
/// initialized.
rklib_fun rk_pure ArenaMark arenastack_mark(const ArenaStack* self) {
  return self->arena_size ? arena_mark(&self->arenas[self->cur]) : (ArenaMark){rk_null};
}
/// @brief Reclaims allocations made after a saved position, retaining backing buffers for reuse.
/// @param self Stack from which the mark was obtained.
/// @param mark A mark from arenastack_mark() on this stack, still valid for its current state and
/// not beyond its current allocation position. A null-position mark (from a zero-initialized
/// stack) rewinds to the start, equivalent to arenastack_clear().
/// @return self, for chaining.
/// @note Invalidates allocations and marks in the discarded region. Rewinding to an empty later
/// arena may make the preceding arena active. Foreign or invalidated non-null marks are
/// unsupported.
rklib_fun ArenaStack* arenastack_rewind_to(ArenaStack* restrict self, ArenaMark mark);

/// @brief `void* arenastack_allocate(size_t nbytes, size_t align, ArenaStack* self)` - Allocates
/// aligned storage, reusing or adding an arena if necessary.
/// @param nbytes Payload size in bytes; may be zero. The size plus align - 1, and its required
/// power-of-two backing-buffer size, must be representable in size_t.
/// @param align Required alignment; must be a nonzero power of two.
/// @param self Initialized or zero-initialized stack to allocate from.
/// @return A non-null aligned pointer on success; failures follow the applicable allocator/arena
/// failure policy rather than returning a normal null failure result.
/// @note A zero-initialized stack is lazily initialized using alloc_ctx. A zero-size request
/// returns an aligned cursor position and consumes only alignment padding; it may initialize or
/// grow the stack. The result may be an end pointer and must not be dereferenced for a zero-size
/// request.
/// @note Existing allocation addresses remain stable when backing buffers or the descriptor vector
/// grow. Unlike generic alloc_allocate(), this direct API does not normalize zero-size requests to
/// NULL; such a zero-size position must not be passed to alloc_deallocate()/alloc_reallocate().
/// @see arenastack_new
/// @see arenastack_to_alloc
rklib_fun void*       arenastack_allocate(size_t nbytes, size_t align, ArenaStack* self);

/// @brief `T* arenastack_new(T, size_t count, ArenaStack* arena_stack)` - Allocates raw storage for
/// count elements with the type's required alignment.
/// @param T Element type.
/// @param count Element count; may be zero. The byte-size calculation and backing-size requirements
/// of arenastack_allocate() must be representable.
/// @param arena_stack Stack to allocate from.
/// @return A typed pointer to the allocated storage.
/// @note Does not invoke C++ constructors. Zero count follows arenastack_allocate() semantics.
/// @see arenastack_allocate
#define arenastack_new(T, count, arena_stack) RKI_ARENASTACK_NEW(T, count, arena_stack)

/// @brief `T* arenastack_new_aligned(T, size_t count, size_t align, ArenaStack* arena_stack)` -
/// Like arenastack_new(), with explicitly requested alignment.
/// @param T Element type.
/// @param count Element count; may be zero. Byte-size and backing-size calculations must be
/// representable.
/// @param align A nonzero power of two, at least the alignment required by T.
/// @param arena_stack Stack to allocate from.
/// @return A typed pointer to the allocated raw storage.
/// @see arenastack_new
/// @see arenastack_allocate
#define arenastack_new_aligned(T, count, align, arena_stack)                                       \
  RKI_ARENASTACK_ALIGNED_NEW(T, count, align, arena_stack)

/// @brief `void* arenastack_try_resize_top(size_t old_size, size_t new_size, ArenaStack* self)` -
/// Resizes the active arena's top allocation without moving its start, like
/// arena_try_resize_top().
/// @param old_size Current payload size of the active arena's top allocation in bytes; may be zero
/// for a valid zero-size position at the active cursor.
/// @param new_size Desired payload size in bytes; may be zero.
/// @param self Stack owning the top allocation or zero-size position. For a zero-initialized or
/// released stack, `old_size` must be zero and the request fails, including when `new_size` is
/// zero.
/// @return The unchanged start pointer on success, including when `new_size` is zero; `NULL` if
/// the requested size does not fit in the active arena.
/// @note Failure leaves the stack unchanged. Never moves data, adds an arena, or invokes failure
/// handlers. A zero new size reclaims the payload but not preceding alignment padding; the
/// returned pointer then denotes only a zero-size position and must not be dereferenced.
/// @note Only the active arena's top can be resized. An allocation that ends an earlier arena is
/// not the stack's top, even if it was the most recent allocation before the stack grew.
/// @note Operates on the implicit top allocation, so `old_size` correctly describing it is a
/// precondition, not a checked failure. To resize a specific allocation that may not be on top,
/// use arenastack_try_resize() or arenastack_try_extend().
/// @see arenastack_try_resize
rklib_fun void* arenastack_try_resize_top(size_t old_size, size_t new_size, ArenaStack* self);

/// @brief `void* arenastack_try_resize(void* ptr, size_t old_size, size_t new_size,
/// ArenaStack* self)` - Attempts to resize a specific allocation in place without moving it, like
/// arena_try_resize(); it can grow or shrink.
/// @param ptr Non-null pointer to an allocation or zero-size position owned by this stack; it need
/// not be the top allocation.
/// @param old_size Current payload size in bytes; must match the allocation.
/// @param new_size Desired payload size in bytes; may be zero.
/// @param self Stack owning the allocation.
/// @return `ptr` on success, including when `new_size` is zero. `NULL` if `ptr` is not the active
/// arena's top allocation (including the top of an earlier arena), the requested size does not
/// fit in the active arena, or the stack is zero-initialized or released.
/// @note Failure leaves the stack unchanged; a non-top allocation is an ordinary failure, not a
/// contract violation. Ownership and a correct `old_size` remain preconditions. Never moves data
/// or adds an arena. Preserves the retained bytes. A zero new size reclaims the payload but not
/// preceding alignment padding; the returned pointer then denotes only a zero-size position and
/// must not be dereferenced.
/// @see arenastack_try_extend
/// @see arenastack_try_resize_top
rklib_fun void* arenastack_try_resize(void* ptr, size_t old_size, size_t new_size,
                                      ArenaStack* self);

/// @brief `T* arenastack_try_extend(T* ptr, size_t old_count, size_t new_count,
/// ArenaStack* arena_stack)` - Typed arenastack_try_resize() using element counts; despite the
/// name, it can also shrink.
/// @param ptr Non-null pointer to an allocation or zero-size position owned by this stack; it need
/// not be the top allocation.
/// @param old_count Current payload element count; must match the allocation.
/// @param new_count Desired element count; may be zero. Byte-size calculations must be
/// representable.
/// @param arena_stack Stack owning the allocation.
/// @return `ptr` on success, cast to the same pointer type, including when `new_count` is zero.
/// `NULL` under the same conditions as arenastack_try_resize(), including a non-top `ptr`.
/// @note Inherits arenastack_try_resize() semantics. Does not construct or destroy C++ objects.
/// @see arenastack_try_resize
#define arenastack_try_extend(ptr, old_count, new_count, arena_stack)                              \
  ((typeof(ptr))arenastack_try_resize((ptr), sizeof_n(*(ptr), old_count),                          \
                                      sizeof_n(*(ptr), new_count), (arena_stack)))

/// @brief Internal allocation callback for the ArenaStack allocator adapter.
/// @note Generic alloc_* wrappers normalize zero-size and null-pointer cases before dispatch:
/// allocation callbacks receive positive sizes; reallocation/deallocation callbacks receive valid
/// non-null existing allocations with matching sizes and alignment, and reallocation receives a
/// positive new size. Allocation and reallocation callbacks must return valid storage or handle
/// failure locally. The allocation callback also serves arenastack_allocate(), whose direct
/// zero-size contract is broader.
rklib_fun alloc_allocation_f   rki_arenastack_allocate;
rklib_fun alloc_reallocation_f rki_arenastack_reallocate;
rklib_fun alloc_deallocation_f rki_arenastack_deallocate;
static const AllocatorVTable arenastack_allocator_vtable = {.alloc_f   = rki_arenastack_allocate,
                                                            .realloc_f = rki_arenastack_reallocate,
                                                            .dealloc_f = rki_arenastack_deallocate};

/// @brief Returns an Allocator handle borrowing the ArenaStack as its context.
/// @param self Initialized or zero-initialized stack; must remain at the same address while used.
/// @return An adapter allocating within the stack, not its backing allocator.
/// @note Only useful with custom allocators enabled: otherwise alloc_* wrappers accept no explicit
/// allocator and alloc_ctx cannot be replaced, so the handle cannot be passed anywhere.
/// @note Generic alloc_* wrappers return NULL for zero-size allocation, deallocate and return NULL
/// for zero-size reallocation, route null-input reallocation to allocation, and ignore null
/// deallocation. These cases are normalized before callback dispatch.
/// @note Only the active arena's top allocation can reclaim payload storage on deallocation or
/// resize. Other deallocations and shrinks reclaim no storage. Growth may allocate and copy,
/// retaining the old storage until bulk reclamation. Preceding alignment padding is not reclaimed
/// by deallocation.
/// @note Clearing, rewinding, or releasing must not discard storage still used through this handle.
/// @see arenastack_allocator
static_fun rk_const Allocator arenastack_to_alloc(ArenaStack* self) {
  return (Allocator){.vtab = &arenastack_allocator_vtable, .ctx = self};
}

#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#define RKI_ARENASTACK_ALIGNED_NEW(T, count, align, arena_stack)                                   \
  ((typeof(T)*)(alloc_log_new(), rk_assert_valid_align(T, align),                                  \
                arenastack_allocate(sizeof_n(T, count), align, arena_stack)))

#define RKI_ARENASTACK_NEW(T, count, arena_stack)                                                  \
  ((typeof(T)*)(alloc_log_new(), arenastack_allocate(sizeof_n(T, count), alignof(T), arena_stack)))

rklib_fun void arenastack_release(ArenaStack* self) {
  RK_IFALLOC(Allocator alloc = vec_allocator(self->arenas);)
  vec_foreach(self->arenas, arena) {
    alloc_deallocate(arena->beg, (size_t)(arena->end - arena->beg), align_max RK_IFALLOC(, alloc));
  }
  vec_release(self->arenas);
  self->arena_size = 0, self->cur = 0;
}

rklib_fun ArenaStack* arenastack_clear(ArenaStack* self) {
  if rk_unlikely (!self->arena_size) { return self; }
  for (size_t cur = self->cur, i = 0; i <= cur; ++i) { arena_clear(&self->arenas[i]); }
  return self->cur = 0, self;
}

rklib_fun ArenaStack* arenastack_rewind_to(ArenaStack* restrict self, ArenaMark mark) {
  const unsigned char* ptr = mark.pos;
  // A null mark was taken before lazy initialization, i.e. at the starting position.
  if rk_unlikely (!ptr) { return arenastack_clear(self); }
  for (size_t i = self->cur + 1; i-- > 0;) {
    Arena* arena = &self->arenas[i];
    if (ptr == arena->cur || rk_ptr_in_range(ptr, arena->beg, arena->cur)) {
      arena_rewind_to(arena, mark);
      self->cur = i - (i > 0 && arena_is_empty(arena));
      return self;
    }
    arena_clear(arena);
  }
  rk_assert(0 && "Pointer was not allocated by this stack");
  unreachable();
}
// The end-position check alone is not enough here: when the active arena is empty and its buffer
// directly follows the previous arena's buffer, a top allocation of the previous arena also ends at
// the active cursor. Require the allocation to start inside the active arena as well; a zero-size
// position may equal the cursor.
rklib_fun rk_pure bool rki_arenastack_is_active_top(const Arena* arena, const void* ptr,
                                                    size_t size) {
  return (uptr)ptr >= (uptr)arena->beg && arena_is_top_allocation(arena, ptr, size);
}

rklib_fun rk_alloc_size(2) void* arenastack_try_resize_top(size_t old_size, size_t new_size,
                                                           ArenaStack* self) {
  if rk_unlikely (!self->arena_size) { return rk_null; }
  return arena_try_resize_top(old_size, new_size, &self->arenas[self->cur]);
}

rklib_fun rk_alloc_size(3) void* arenastack_try_resize(void* ptr, size_t old_size, size_t new_size,
                                                       ArenaStack* self) {
  if (!self->arena_size || !rki_arenastack_is_active_top(&self->arenas[self->cur], ptr, old_size)) {
    return rk_null;
  }
  return arenastack_try_resize_top(old_size, new_size, self);
}

rklib_fun ArenaStack rki_arenastack_init(size_t cap RK_IFALLOC(, Allocator alloc)) {
  RKI_assert_allocator_valid(alloc);
  cap = stdc_bit_ceil(cap); /*1 if cap==0*/
  return (ArenaStack){
      .arena_size = cap,
      .arenas     = vec_init_list(
          Arena, RK_IFALLOC(alloc, ) arena_init(
                     (unsigned char*)alloc_allocate(cap, align_max RK_IFALLOC(, alloc)), cap)),
      .cur = 0};
}
#define RKI_ARENASTACK_INIT2(cap, alloc)                                                           \
  RKI_REQUIRE_CUSTOM_ALLOCATORS(rki_arenastack_init(cap, alloc))
#define RKI_ARENASTACK_INIT1(cap) rki_arenastack_init(cap RK_IFALLOC(, alloc_ctx))

#define RKI_ARENA_ALLOC_INIT(_SIZE, _ALIGN, _ALLOC)                                                \
  arena_init((unsigned char*)alloc_allocate(_SIZE, _ALIGN RK_IFALLOC(, _ALLOC)), _SIZE)

rklib_fun rk_alloc_alignsize(2, 1) void* rki_arenastack_allocate(size_t nbytes, size_t align,
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
                          RKI_ARENA_ALLOC_INIT(stdc_bit_ceil(rk_MAX(needed, self->arena_size)),
                                               align_max, vec_allocator(self->arenas)));
  return arena_allocate(nbytes, align, &self->arenas[self->cur]);
}

rklib_fun void rki_arenastack_deallocate(void* ptr, size_t old_size, size_t align rk_unused,
                                         void* ctx) {
  ArenaStack* self  = (ArenaStack*)ctx;
  Arena*      arena = &self->arenas[self->cur];
  if (rki_arenastack_is_active_top(arena, ptr, old_size)) {
    (void)arena_try_resize_top(old_size, 0, arena);
  }
}

rklib_fun rk_alloc_alignsize(4, 3) void* rki_arenastack_reallocate(void* ptr, size_t old_size,
                                                                   size_t new_size, size_t align,
                                                                   void* ctx) {
  rk_assert_align_pow2(align);
  ArenaStack* self  = (ArenaStack*)ctx;
  Arena*      arena = &self->arenas[self->cur];

  if ((rki_arenastack_is_active_top(arena, ptr, old_size)
       && arena_try_resize_top(old_size, new_size, arena))
      || new_size <= old_size) {
    return ptr;
  }

  void* res = arenastack_allocate(new_size, align, self);
  rk_memcpy(res, ptr, old_size);
  return res;
}
rklib_fun rk_alloc_alignsize(2, 1) void* arenastack_allocate(size_t nbytes, size_t align,
                                                             ArenaStack* self) {
  return rki_arenastack_allocate(nbytes, align, self);
}

#undef RKI_ARENA_ALLOC_INIT

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
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
