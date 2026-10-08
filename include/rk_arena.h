// SPDX-License-Identifier: MIT
/// @file rk_arena.h
/// @version 1.0.0
/// @defgroup rk_arena Arena Allocator Interface
/// @brief Linear arena allocation over borrowed storage, with Allocator integration.
/// @details Allocations advance a cursor. Clearing or rewinding reclaims storage in bulk without
/// releasing the backing buffer. Direct arena operations and generic alloc_* wrappers have
/// different zero-size semantics; see arena_allocate() and arena_to_alloc_static().
/// @note Concurrent access involving cursor changes requires external synchronization. Separate
/// arenas may be used independently when their backing storage does not interfere.
/// @see rk_alloc.h
/// @see rk_defs.h
/// @{
#ifndef RK_ARENA_H
#define RK_ARENA_H
#include "rk_alloc.h"
RKI_HEADER_BEGIN
RKI_IGNWARN_CLANG_BEG("-Wreturn-type-c-linkage")

/// @brief Non-owning linear arena over a fixed backing buffer.
/// @details Allocation advances the cursor; marks, rewinding, and clearing allow bulk reuse.
/// @note The backing buffer must remain valid while the arena or its allocations are used.
/// Copying an Arena copies its cursor state and pointers; it does not copy the backing storage.
/// All Arena pointer arguments must be non-null and refer to initialized objects.
/// A zero-initialized Arena has no backing buffer; queries, clearing, marking, and
/// allocation/resize attempts support this state.
typedef struct Arena {
  unsigned char* beg; ///< Start of arena memory block
  unsigned char* cur; ///< Current Position in arena memory block
  unsigned char* end; ///< End of arena memory block
} Arena;

/// @brief Initializes an arena over borrowed byte storage.
/// @param arr Backing buffer, readable and writable for `len` bytes, or `NULL` for no backing
/// storage.
/// @param len Buffer length in bytes. Must not extend beyond the backing buffer.
/// @return An arena with its cursor at the beginning. A null buffer produces a null-backed arena.
/// @note No storage is allocated or owned by the arena; a null buffer ignores `len`.
rklib_fun rk_const Arena arena_init(unsigned char* arr, size_t len) {
  return (Arena){.beg = arr, .cur = arr, .end = arr ? arr + len : 0};
}

/// @brief `Arena arena_init_static(unsigned char arr[])` - Produces an initializer for an arena
/// using a byte array as borrowed backing storage; capacity is determined by `sizeof(arr)`.
/// @param array_non_compound_literal The backing byte array; must be an actual array, not a
/// pointer.
/// @return An initializer for a new Arena.
/// @warning Do not use with compound literals; use arena_init() instead.
/// @note Can be used for static initialization when the array has a suitable storage duration.
#define arena_init_static(array_non_compound_literal)                                              \
  RKI_ARENA_INIT_STATIC(array_non_compound_literal)

/// @brief Resets the cursor to the beginning, reclaiming all consumed storage, including padding.
/// @return `self`, for chaining.
/// @note Invalidates all previous allocations and marks. Does not release or clear the backing
/// buffer. A zero-initialized, null-backed arena is supported.
rklib_fun Arena*         arena_clear(Arena* self) { return self->cur = self->beg, self; }

/// @brief Returns the backing-buffer capacity in bytes, or zero for a null-backed arena.
rklib_fun rk_pure size_t arena_cap(const Arena* self) {
  return rk_likely(self->beg) ? (size_t)(self->end - self->beg) : 0;
}

/// @brief Returns consumed storage in bytes, including alignment padding and unreclaimed storage.
/// @note Returns zero for a null-backed arena; this is not a count of live allocation bytes.
rklib_fun rk_pure size_t arena_used(const Arena* self) {
  return rk_likely(self->beg) ? (size_t)(self->cur - self->beg) : 0;
}

/// @brief Returns unused backing-buffer bytes, or zero for a null-backed arena.
/// @note A request may require additional alignment padding, so this is not necessarily the largest
/// payload that can be allocated with a particular alignment.
rklib_fun rk_pure size_t arena_remaining(const Arena* self) {
  return rk_likely(self->beg) ? (size_t)(self->end - self->cur) : 0;
}

/// @brief Returns whether the cursor is at the beginning, or true for a null-backed arena.
/// @note Tests consumed storage, not the number of live allocations. Padding can keep the result
/// false after payload storage has been reclaimed.
rklib_fun rk_pure bool arena_is_empty(const Arena* self) {
  return rk_likely(self->beg) ? self->cur == self->beg : true;
}

typedef struct ArenaMark { unsigned char* pos; } ArenaMark;

/// @brief Saves the current cursor as a marker for arena_rewind_to().
/// @return A marker for this arena's current position; a null-backed arena yields a null-position
/// mark.
/// @note A mark neither allocates storage nor keeps allocations alive.
rklib_fun rk_pure ArenaMark arena_mark(const Arena* self) { return (ArenaMark){.pos = self->cur}; }

/// @brief Restores a saved cursor position, reclaiming storage consumed after that position.
/// @param self Arena from which the mark was obtained.
/// @param mark A mark obtained from arena_mark() on this arena, valid for its current state and
/// not ahead of its current cursor.
/// @return `self`, for chaining.
/// @note Invalidates allocations discarded by the rewind and marks into the discarded region.
/// Rewinding a null-backed arena to its own null-position mark is supported. A null-position mark
/// used after the arena was given backing storage is stale, and therefore a contract violation.
rklib_fun Arena*            arena_rewind_to(Arena* self, ArenaMark mark);

/// @brief Returns whether the given allocation ends exactly at the current cursor.
/// @param self Arena owning the allocation.
/// @param ptr Non-null pointer to a valid allocation or zero-size position returned by this arena.
/// @param size Current allocation size in bytes; must describe valid storage at `ptr`.
/// @return True if `ptr + size` equals the current cursor, false otherwise.
/// @note This is an end-position check, not an ownership or allocation-history validator.
rklib_fun rk_pure bool arena_is_top_allocation(const Arena* self, const void* ptr, size_t size) {
  return (const unsigned char*)ptr + size == self->cur;
}

/// @brief `void* arena_allocate(size_t nbytes, size_t align, Arena* self)` - Allocates aligned
/// storage from the arena. Invokes `RK_ARENA_FAIL` on failure; the default handler aborts.
/// @param nbytes Requested payload size in bytes; may be zero.
/// @param align Required alignment; must be a nonzero power of two.
/// @param self Arena to allocate from; may be null-backed, in which case the request fails.
/// @return A non-null aligned pointer on success, provided any failure override preserves the
/// contract.
/// @note Zero-size requests reserve no payload but advance the cursor by any required alignment
/// padding. Their result may equal the end pointer and must not be dereferenced. Alignment padding
/// must fit.
/// @note Unlike the generic alloc_allocate() wrapper, this direct API does not return null for zero
/// bytes. Such a zero-size position must not be passed to alloc_deallocate()/alloc_reallocate(),
/// which require `NULL` for zero-size allocations.
/// @see arena_try_allocate
rklib_fun void* arena_allocate(size_t nbytes, size_t align, Arena* self);

/// @brief `void* arena_try_allocate(size_t nbytes, size_t align, Arena* self)` - Like
/// arena_allocate(), but returns `NULL` on insufficient space or a null backing buffer.
/// @note Failure leaves the cursor unchanged. Zero-size requests succeed if the required alignment
/// padding fits, return an aligned position, and consume only that padding.
/// @see arena_allocate
rklib_fun void* arena_try_allocate(size_t nbytes, size_t align, Arena* self);

/// @brief `void* arena_resize_top(size_t old_size, size_t new_size, Arena* self)` - Resizes the
/// current top allocation without moving its start. Invokes `RK_ARENA_FAIL` on failure.
/// @param old_size Current payload size of the top allocation in bytes; may be zero for a valid
/// zero-size position at the current cursor.
/// @param new_size Desired payload size in bytes; may be zero.
/// @param self Arena owning the top allocation or zero-size position. For a null-backed arena,
/// `old_size` must be zero and the request fails, including when `new_size` is zero.
/// @return The unchanged start pointer on success, including when `new_size` is zero.
/// @note A zero new size reclaims the payload but not preceding alignment padding; the returned
/// pointer then denotes only a zero-size position and must not be dereferenced.
/// @note Failure leaves the cursor unchanged before invoking the failure handler. Overrides that
/// resume must preserve the start address, arena state, and resize guarantees; `align_max` passed
/// to the handler is not necessarily the original allocation alignment.
/// @see arena_try_resize_top
rklib_fun void* arena_resize_top(size_t old_size, size_t new_size, Arena* self);

/// @brief `void* arena_try_resize_top(size_t old_size, size_t new_size, Arena* self)` - Like
/// arena_resize_top(), but returns `NULL` if the requested size does not fit or the arena is
/// null-backed.
/// @note Failure leaves the cursor unchanged. Success returns the unchanged start pointer even for
/// zero new size. The top-allocation precondition still applies; a null-backed arena requires
/// `old_size == 0`.
/// @note Operates on the implicit top allocation, so `old_size` correctly describing it is a
/// precondition, not a checked failure. To resize a specific allocation that may not be on top,
/// use arena_try_resize() or arena_try_extend().
/// @see arena_resize_top
/// @see arena_try_resize
rklib_fun void* arena_try_resize_top(size_t old_size, size_t new_size, Arena* self);

/// @brief `void* arena_try_resize(void* ptr, size_t old_size, size_t new_size, Arena* self)` -
/// Attempts to resize a specific allocation in place without moving it; it can grow or shrink.
/// @param ptr Non-null pointer to an allocation or zero-size position owned by this arena; it need
/// not be the top allocation.
/// @param old_size Current payload size in bytes; must match the allocation.
/// @param new_size Desired payload size in bytes; may be zero.
/// @param self Arena owning the allocation.
/// @return `ptr` on success, including when `new_size` is zero. `NULL` if `ptr` is not the top
/// allocation, the requested size does not fit, or the arena is null-backed.
/// @note Failure leaves the arena unchanged; a non-top allocation is an ordinary failure, not a
/// contract violation. Ownership and a correct `old_size` remain preconditions. Preserves the
/// retained bytes. A zero new size reclaims the payload but not preceding alignment padding; the
/// returned pointer then denotes only a zero-size position and must not be dereferenced.
/// @see arena_try_extend
/// @see arena_try_resize_top
rklib_fun void* arena_try_resize(void* ptr, size_t old_size, size_t new_size, Arena* self);

/// @brief `T* arena_new(T, size_t count, Arena* arena)` - Allocates storage for `count` elements
/// with the type's required alignment.
/// @param T Element type.
/// @param count Element count; may be zero. The byte-size calculation must be representable.
/// @param arena Arena to allocate from.
/// @return A typed pointer to the allocated storage.
/// @note Allocates raw storage; does not invoke C++ constructors. Uses arena_allocate() zero-size
/// and failure semantics.
/// @see arena_allocate
#define arena_new(T, count, arena)                RKI_ARENA_NEW(T, count, arena)

/// @brief `T* arena_try_new(T, size_t count, Arena* arena)` - Like arena_new(), but returns
/// `NULL` on allocation failure without invoking the failure handler.
/// @note Zero count follows arena_try_allocate() semantics and may return a non-null pointer.
/// @see arena_new
/// @see arena_try_allocate
#define arena_try_new(T, count, arena)            arena_try_new_aligned(T, count, alignof(T), arena)

/// @brief `T* arena_new_aligned(T, size_t count, size_t align, Arena* arena)` - Like arena_new(),
/// with explicitly requested alignment.
/// @param T Element type.
/// @param count Element count; may be zero. The byte-size calculation must be representable.
/// @param align A nonzero power of two, at least the alignment required by T.
/// @param arena Arena to allocate from.
/// @return A typed pointer to the allocated storage.
/// @see arena_new
/// @see arena_allocate
#define arena_new_aligned(T, count, align, arena) RKI_ARENA_ALIGNED_NEW(T, count, align, arena)

/// @brief `T* arena_try_new_aligned(T, size_t count, size_t align, Arena* arena)` - Like
/// arena_new_aligned(), but returns `NULL` on allocation failure without invoking the failure
/// handler.
/// @note Zero count follows arena_try_allocate() semantics and may return a non-null pointer.
/// @see arena_new_aligned
/// @see arena_try_allocate
#define arena_try_new_aligned(T, count, align, arena)                                              \
  ((T*)(alloc_log_new(), rk_assert_valid_align(T, align),                                          \
        arena_try_allocate(sizeof_n(T, count), align, arena)))

/// @brief `T* arena_extend(T* ptr, size_t old_count, size_t new_count, Arena* arena)` - Like
/// arena_try_extend(), but invokes `RK_ARENA_FAIL` on failure, including when `ptr` is not the top
/// allocation.
/// @return `ptr` on success, cast to the same pointer type, including when `new_count` is zero.
/// @note Failure leaves the cursor unchanged before invoking the failure handler.
/// @see arena_try_extend
#define arena_extend(ptr, old_count, new_count, arena)                                             \
  ((typeof(ptr))rki_arena_extend(ptr, sizeof_n(*(ptr), old_count), sizeof_n(*(ptr), new_count),    \
                                 arena))

/// @brief `T* arena_try_extend(T* ptr, size_t old_count, size_t new_count, Arena* arena)` -
/// Typed arena_try_resize() using element counts; despite the name, it can also shrink.
/// @param ptr Non-null pointer to an allocation or zero-size position owned by this arena; it need
/// not be the top allocation.
/// @param old_count Current payload element count; must match the allocation.
/// @param new_count Desired element count; may be zero. Byte-size calculations must be
/// representable.
/// @param arena Arena owning the allocation.
/// @return `ptr` on success, cast to the same pointer type, including when `new_count` is zero.
/// `NULL` under the same conditions as arena_try_resize(), including a non-top `ptr`.
/// @note Inherits arena_try_resize() semantics. Does not construct or destroy C++ objects.
/// @see arena_try_resize
#define arena_try_extend(ptr, old_count, new_count, arena)                                         \
  ((typeof(ptr))arena_try_resize(ptr, sizeof_n(*(ptr), old_count), sizeof_n(*(ptr), new_count),    \
                                 arena))

/// @brief Internal allocation callback for the Arena allocator adapter.
/// @note Generic alloc_* wrappers handle zero-size and null-pointer cases before dispatch:
/// allocation callbacks receive positive sizes; reallocation/deallocation callbacks receive valid
/// non-null existing allocations with matching sizes and alignment, and reallocation receives a
/// positive new size. Allocation and reallocation callbacks must return valid storage or handle
/// failure locally. The allocation callback also serves arena_allocate(), whose direct zero-size
/// contract is broader.
rklib_fun alloc_allocation_f   rki_arena_allocate;
rklib_fun alloc_reallocation_f rki_arena_reallocate;
rklib_fun alloc_deallocation_f rki_arena_deallocate;
static const AllocatorVTable   arena_allocator_vtable = {.alloc_f   = rki_arena_allocate,
                                                         .realloc_f = rki_arena_reallocate,
                                                         .dealloc_f = rki_arena_deallocate};

/// @brief `Allocator arena_to_alloc_static(Arena* arena)` - Produces an Allocator initializer
/// borrowing an Arena as its context.
/// @note The Arena must remain at the same address and its backing buffer must remain valid while
/// this handle or allocations using it remain in use.
/// @note Generic alloc_* wrappers normalize zero-size and null-pointer cases before vtable
/// dispatch. Reallocation resizes top allocations in place where possible; non-top shrinks reclaim
/// no storage, and moving growth retains the old storage until it can be reclaimed by rewinding or
/// clearing. Deallocation reclaims only a top allocation's payload; preceding padding remains
/// consumed.
/// @note Clearing or rewinding must not discard storage still accessed by objects using this
/// handle.
/// @see arena_to_alloc
#define arena_to_alloc_static(arena) {.vtab = &arena_allocator_vtable, .ctx = (arena)}

/// @brief Returns an Allocator handle borrowing the given Arena as its context.
/// @note Does not allocate, copy, or take ownership of the arena or its backing buffer.
/// @note Only useful with custom allocators enabled: otherwise alloc_* wrappers accept no explicit
/// allocator and alloc_ctx cannot be replaced, so the handle cannot be passed anywhere. The same
/// applies to arena_to_alloc_static() and arr_allocator.
/// @see arena_to_alloc_static
static_fun rk_const Allocator arena_to_alloc(Arena* arena) {
  return (Allocator)arena_to_alloc_static(arena);
}

/// @brief Defines a type containing an Allocator handle, an Arena, and an embedded byte buffer.
/// @param size Positive compile-time buffer size in bytes.
/// @note Initialization stores pointers into the object itself. Keep the initialized object at its
/// original address; copying it does not retarget those pointers to the copy.
/// @note Only useful with custom allocators enabled; see arena_to_alloc().
/// @see arr_allocator_init
/// @see arr_allocator_create
#define arr_allocator(size)                                                                      \
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

/// @brief Produces an initializer for an arr_allocator object using its address.
/// @param self Address of the object being initialized.
/// @note This is an initializer, not a runtime reinitialization function. The embedded buffer is
/// borrowed by the embedded Arena, and the Allocator context points to that Arena.
#define arr_allocator_init(self)                                                                   \
  {.vtab = &arena_allocator_vtable, .ctx = &(self)->arena, .arena = arena_init_static((self)->arr)}

/// @brief Declares and initializes an allocator with an embedded arena and fixed-size backing
/// buffer.
/// @param name Name of the object to declare.
/// @param size Positive compile-time buffer size in bytes.
/// @note Use `name.alloc` as the Allocator handle. The object's storage duration follows its
/// declaration context; keep it at its initialized address and alive while dependent allocations
/// are used.
/// @code
/// arr_allocator_create(temp_alloc, 4096);
/// int* ptr = alloc_new(int, 10, temp_alloc.alloc);
/// @endcode
/// @see arr_allocator
#define arr_allocator_create(name, size) arr_allocator(size) name = arr_allocator_init(&name)

#pragma region implementation
////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

rklib_fun Arena* arena_rewind_to(Arena* self, ArenaMark mark) {
  if (mark.pos == self->cur) { return self; }
  rk_assert(rk_ptr_in_range(mark.pos, self->beg, self->cur) && "Pointer is outside this Arena");
  self->cur = mark.pos;
  return self;
}

rklib_fun rk_alloc_alignsize(2, 1) void* arena_try_allocate(size_t nbytes, size_t align,
                                                            Arena* self) {
  if rk_unlikely (!self->cur) { return rk_null; }
  size_t pad = rk_align_pad(self->cur, align), avail = arena_remaining(self);
  if (avail < pad || avail - pad < nbytes) { return rk_null; }
  unsigned char* ptr = self->cur + pad;
  self->cur          = ptr + nbytes;
  return ptr;
}

rklib_fun rk_alloc_size(2) void* arena_try_resize_top(size_t old_size, size_t new_size,
                                                      Arena* self) {
  if (arena_remaining(self) + old_size < new_size) { return rk_null; }
  if rk_unlikely (!self->cur) { return rk_null; }
  unsigned char* ptr = self->cur - old_size;
  self->cur          = ptr + new_size;
  return ptr;
}

rklib_fun rk_alloc_size(2) void* arena_resize_top(size_t old_size, size_t new_size, Arena* self) {
  void* res = arena_try_resize_top(old_size, new_size, self);
  RK_ARENA_FAIL(res, self, (self->cur ? self->cur - old_size : rk_null), align_max, new_size);
  return res;
}

rklib_fun rk_alloc_alignsize(2, 1) void* rki_arena_allocate(size_t nbytes, size_t align,
                                                            void* ctx) {
  void* ptr = arena_try_allocate(nbytes, align, (Arena*)ctx);
  RK_ARENA_FAIL(ptr, (Arena*)ctx, rk_null, align, nbytes);
  return ptr;
}

rklib_fun void rki_arena_deallocate(void* ptr, size_t old_size, size_t align rk_unused, void* ctx) {
  if (arena_is_top_allocation((Arena*)ctx, ptr, old_size)) {
    (void)arena_try_resize_top(old_size, 0, (Arena*)ctx);
  }
}

rklib_fun rk_alloc_alignsize(4, 3) void* rki_arena_reallocate(void* ptr, size_t old_size,
                                                              size_t new_size, size_t align,
                                                              void* ctx) {
  rk_assert_align_pow2(align);
  Arena* self = (Arena*)ctx;

  if ((arena_is_top_allocation(self, ptr, old_size)
       && arena_try_resize_top(old_size, new_size, self))
      || new_size <= old_size) {
    return ptr;
  }

  void* res = rki_arena_allocate(new_size, align, self);
  rk_memcpy(res, ptr, old_size);
  return res;
}

rklib_fun rk_alloc_alignsize(2, 1) void* arena_allocate(size_t nbytes, size_t align, Arena* self) {
  return rki_arena_allocate(nbytes, align, self);
}
rklib_fun rk_alloc_size(3) void* arena_try_resize(void* ptr, size_t old_size, size_t new_size,
                                                  Arena* self) {
  if (!arena_is_top_allocation(self, ptr, old_size)) { return rk_null; }
  return arena_try_resize_top(old_size, new_size, self);
}

rklib_fun rk_alloc_size(3) void* rki_arena_extend(void* ptr, size_t old_size, size_t new_size,
                                                  Arena* self) {
  void* r = arena_try_resize(ptr, old_size, new_size, self);
  RK_ARENA_FAIL(r, self, ptr, align_max, new_size);
  return r;
}

#define RKI_ARENA_INIT_STATIC(arr)                                                                 \
  {.beg = (arr) + rk_ensure_valid_storage_type(arr), .cur = (arr), .end = (arr) + sizeof(arr)}

#define RKI_ARENA_ALIGNED_NEW(T, count, align, arena)                                              \
  ((typeof(T)*)(alloc_log_new(), rk_assert_valid_align(T, align),                                  \
                arena_allocate(sizeof_n(T, count), align, arena)))
#define RKI_ARENA_NEW(T, count, arena)                                                             \
  ((typeof(T)*)(alloc_log_new(), arena_allocate(sizeof_n(T, count), alignof(T), arena)))
RKI_IGNWARN_CLANG_END()

/// @endcond
#pragma endregion implementation
RKI_HEADER_END
/// @}
#endif // RK_ARENA_H

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
