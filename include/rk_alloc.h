// SPDX-License-Identifier: MIT
/// @file rk_alloc.h
/// @version 1.0
/// @defgroup rk_alloc Allocator Interface
/// @brief Customizable memory allocator abstraction for C.
///
/// Provides an allocator interface built around `Allocator` — a vtable pointer plus an optional
/// context pointer. Two predefined allocators are provided: `alloc_malloc_allocator` and
/// `alloc_page_allocator`. Custom allocators can be created by filling
/// an `AllocatorVTable` and constructing an `Allocator`.
///
/// Allocation failures are handled inside the allocator, not at call sites. The provided allocators
/// invoke the overridable failure macros from `rk_config.h` (`RK_MALLOC_FAIL`, `RK_MMAP_FAIL`,
/// etc.), which by default assert and abort. Callers never need to NULL-check allocation results.
///
/// `RK_CUSTOM_ALLOCATORS` controls whether allocators are threaded through objects — see
/// `rk_config.h`. When disabled, per-object `Allocator` fields, custom-allocator arguments, and
/// function-pointer dispatch are compiled out.
///
/// When `RK_ALLOC_MULTITHREADED == 1`, `alloc_ctx` has thread-local storage duration, giving each
/// thread its own construction-time default allocator.
/// @{
#ifndef RK_ALLOC_H
#define RK_ALLOC_H

#ifndef _MSC_VER
# include <sys/mman.h>
# include <unistd.h>
// todo important MAP_ANONYMOUS not available without the right macros

# ifndef MAP_ANONYMOUS
#  ifdef MAP_ANON
#   define MAP_ANONYMOUS MAP_ANON
#  elif defined(__linux__)
#   define MAP_ANONYMOUS 0x20
#  else
#   error "RK_MAP_ANONYMOUS unknown on this platform, change posix feature test macro"
#  endif
# endif
#endif
#include "rk_defs.h"
RK_HEADER_BEGIN

/// @brief Allocation logging macros. Emit a tagged source location to `stderr` when `RKLIB_DEBUG
/// defined`; expand to nothing otherwise. Can be used by custom allocators to get the same logging
/// behaviour as the built-in ones.
#define alloc_log_new                  rk_log("[alloc]  %s:%d ", __FILE__, __LINE__)
#define alloc_log_renew                rk_log("[renew]  %s:%d ", __FILE__, __LINE__)
#define alloc_log_delete               rk_log("[delete] %s:%d ", __FILE__, __LINE__)
#define rk_allocator_disabled_assert() static_assert_expr(0, "Allocators Disabled")
#define rk_allocator_disabled()        ((Allocator){.ctx = (void*)rk_allocator_disabled_assert()})

#if RK_CUSTOM_ALLOCATORS
# define rk_disable_if(...) __VA_ARGS__
#else
# define rk_disable_if(...) ((void*)rk_allocator_disabled_assert())
#endif

/// @struct Allocator
/// @brief General-purpose allocator handle: a vtable pointer plus an optional context pointer. Pass
/// by value to init functions; pass by pointer to allocator-generic macros.
///
/// When custom allocators are enabled, rklib macros that accept an optional allocator argument
/// default to `alloc_ctx`. Objects capture that allocator when initialised, so changing `alloc_ctx`
/// affects only subsequently created objects.
///
/// Two predefined `Allocator` instances are provided:
///   - `alloc_malloc_allocator` — thin wrappers over `malloc`/`free` (or `_aligned_malloc` on MSVC
///     for over-aligned types). This is the default `alloc_ctx`.
///   - `alloc_page_allocator` — OS page allocation (`mmap` / `VirtualAlloc`). All allocations are
///     page-aligned; alignments larger than the page size are not supported.
///
/// @note Custom allocators must handle failures locally (via the failure macros in `rk_config.h`).
/// Returning `NULL` from an allocator leads to immediate undefined behaviour at the call site.

/// @brief Allocation Function.
/// @param size  Desired size of the allocation in bytes.
/// @param align Desired Alignment of the allocation. Must be a power of two.
/// @param ctx   Allocator context. May be `NULL` depending on the allocator.
/// @return A **valid** pointer to the allocated memory. May only be `NULL` if `size` is zero.
/// @note Allocation failure is expected to be handled locally by the function via the respective
/// failure macros defined in `rk_config.h`.
typedef void*(alloc_allocation_f)(size_t size, size_t align, void* ctx);

/// @brief Reallocation Function.
/// @param old_ptr The pointer to the allocation to be deallocated. If `NULL`, this function shall
/// act like the corresponding `alloc_allocation_f` of the same allocator.
/// @param old_size The size of the allocation to be deallocated. In some allocators such as
/// `alloc_malloc_allocator`, this parameter is discarded.
/// @param new_size The desired new size of the allocation. If this is zero, this function shall act
/// like the corresponding `alloc_deallocation_f` of the same allocator.
/// @param align Desired Alignment of the allocation. Must match the alignment of the corresponding
/// allocation function call.
/// @param ctx      Allocator context. May be `NULL` depending on the allocator.
/// @return A **valid** pointer to the allocated memory. May only be `NULL` if `new_size` is zero.
typedef void*(alloc_reallocation_f)(void* old_ptr, size_t old_size, size_t new_size, size_t align,
                                    void* ctx);

/// @brief Deallocation Function.
/// @param ptr The pointer to the allocation to be freed. If `NULL`, this function shall be a no-op.
/// @param old_size The size of the allocation to be deallocated. In some allocators such as
/// `alloc_malloc_allocator`, this parameter is discarded.
/// @param align Desired Alignment of the allocation. Must match the alignment of the corresponding
/// allocation function call.
/// @param ctx      Allocator context. May be `NULL` depending on the allocator.
typedef void(alloc_deallocation_f)(void* ptr, size_t old_size, size_t align, void* ctx);

/// @brief Vtable for an allocator. Holds function pointers for allocation, deallocation, and
/// reallocation. Shared across all `Allocator` instances that use the same strategy (e.g. all arena
/// allocators share one vtable). Implementations of each slot must follow the contracts described
/// on the `alloc_allocation_f`, `alloc_reallocation_f` and `alloc_deallocation_f` typedefs below.
typedef struct AllocatorVTable {
  alloc_allocation_f*   rk_alloc_alignsize(2, 1) alloc_f;
  alloc_reallocation_f* rk_alloc_alignsize(4, 3) realloc_f;
  alloc_deallocation_f* dealloc_f;
} AllocatorVTable;

typedef struct Allocator {
  const AllocatorVTable* vtab; ///< Vtable pointer
  void*                  ctx;  ///< Optional Context Pointer
} Allocator;

static_fun alloc_allocation_f   RK__malloc_allocate;
static_fun alloc_reallocation_f RK__malloc_reallocate;
static_fun alloc_deallocation_f RK__malloc_deallocate;
static const AllocatorVTable alloc_malloc_allocator_vtable = {.alloc_f   = RK__malloc_allocate,
                                                              .realloc_f = RK__malloc_reallocate,
                                                              .dealloc_f = RK__malloc_deallocate};

/// @brief Default allocator using `malloc`/`free` (or `_aligned_malloc` on MSVC for over-aligned
/// requests). Set as the initial value of `alloc_ctx`.
rk_unused static const Allocator alloc_malloc_allocator
    = {.vtab = &alloc_malloc_allocator_vtable, .ctx = rk_null};

static_fun alloc_allocation_f          RK__page_allocate;
static_fun alloc_reallocation_f        RK__page_reallocate;
static_fun alloc_deallocation_f        RK__page_deallocate;
rk_unused static const AllocatorVTable alloc_page_allocator_vtable
    = {.alloc_f   = RK__page_allocate,
       .realloc_f = RK__page_reallocate,
       .dealloc_f = RK__page_deallocate};

/// @brief Allocator backed by OS page mapping (`mmap` / `VirtualAlloc`). All allocations are
/// page-aligned and zero-initialized. Alignments greater than the system page size are not
/// supported.
rk_unused static const Allocator alloc_page_allocator
    = {.vtab = &alloc_page_allocator_vtable, .ctx = rk_null};

#if RK_CUSTOM_ALLOCATORS
# define RK__ALLOCCTX_STORAGE   extern_var RK_alloc_tl
# define RK__ALLOCCTX_INIT(...) extern_def({__VA_ARGS__})
#else
# define RK__ALLOCCTX_STORAGE   static const
# define RK__ALLOCCTX_INIT(...) = {__VA_ARGS__}
#endif

/// @brief Default allocator used by all rklib macros when no explicit allocator argument is
/// provided. Defaults to `alloc_malloc_allocator`. Objects capture its value when initialised, so
/// replacing it affects only subsequently created objects. When `RK_ALLOC_MULTITHREADED == 1`, it
/// is thread-local. Must always contain a valid, fully initialised `Allocator`.
RK__ALLOCCTX_STORAGE Allocator alloc_ctx RK__ALLOCCTX_INIT(.vtab = &alloc_malloc_allocator_vtable,
                                                           .ctx  = rk_null);

/// @brief `void* alloc_allocate(size_t bytes, size_t align, Allocator alloc = alloc_ctx)` - Raw
/// allocation: allocates `bytes` bytes with the given alignment. Prefer `alloc_new` for typed
/// allocations.
/// @param bytes Number of bytes to allocate
/// @param align Alignment; must be a power of two
/// @param alloc Optional allocator; defaults to `alloc_ctx`
/// @return pointer to the allocated memory. `NULL` iff `bytes` is zero.
#define alloc_allocate(bytes, align, ...)                                                          \
  ((void*)rk_overload(RK__alloc_ALLOCATE, bytes, align, ##__VA_ARGS__))

/// @brief `void* alloc_reallocate(void* ptr, size_t old_bytes, size_t new_bytes, size_t align,
/// Allocator alloc = alloc_ctx)` - Raw reallocation. If `ptr` is `NULL`, behaves like
/// `alloc_allocate`. If `new_bytes` is zero, behaves like `alloc_deallocate`. Prefer `alloc_renew`
/// for typed use.
/// @param ptr       Existing allocation (or `NULL`)
/// @param old_bytes Size of the existing allocation in bytes
/// @param new_bytes Desired new size in bytes
/// @param align     Alignment; must match the original allocation
/// @param alloc     Optional allocator; defaults to `alloc_ctx`
/// @return pointer to the allocated memory. `NULL` iff `new_bytes` is zero.
#define alloc_reallocate(ptr, old_bytes, new_bytes, align, ...)                                    \
  ((void*)rk_overload(RK__alloc_REALLOCATE, ptr, old_bytes, new_bytes, align, ##__VA_ARGS__))

/// @brief `void alloc_deallocate(void* ptr, size_t bytes, size_t align, Allocator alloc =
/// alloc_ctx)` - Raw deallocation. If `ptr` is `NULL`, this is a no-op. Prefer `alloc_delete` for
/// typed use.
/// @param ptr   Pointer to the memory to free (or `NULL`)
/// @param bytes Size of the allocation in bytes
/// @param align Alignment; must match the original allocation
/// @param alloc Optional allocator; defaults to `alloc_ctx`
#define alloc_deallocate(ptr, bytes, align, ...)                                                   \
  ((void)rk_overload(RK__alloc_DEALLOCATE, ptr, bytes, align, ##__VA_ARGS__))

/// @brief `T* alloc_new(T, size_t count, Allocator alloc = alloc_ctx)` - Allocates memory for an
/// array of `count` elements of type `T` using the specified allocator.
/// @param T         The type of elements to allocate
/// @param count     Count of elements to allocate
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
/// @return Pointer to allocated and aligned memory block, cast to `T*`.
#define alloc_new(T, count, ...) ((T*)rk_overload(RK__alloc_NEW, T, count, ##__VA_ARGS__))

/// @brief `T* alloc_renew(T* ptr, size_t old_count, size_t new_count, Allocator alloc = alloc_ctx)`
/// - Resizes (reallocates) memory block to hold `new_count` elements of the same type, for standard
/// alignment according to the Allocator.
/// @param ptr       Pointer to the existing allocated memory
/// @param old_count Number of elements of type T previously allocated
/// @param new_count Number of elements of type T to allocate after resizing
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
/// @return Pointer to the reallocated and aligned memory block, cast to the same pointer type.
/// @warning Must not be used on pointers from over-aligned allocations
#define alloc_renew(ptr, old_count, new_count, ...)                                                \
  ((typeof(ptr))rk_overload(RK__alloc_RENEW, ptr, old_count, new_count, ##__VA_ARGS__))

/// @brief `void alloc_delete(T* ptr, size_t old_count, Allocator alloc = alloc_ctx)` - Deallocates
/// memory.
/// @param ptr       Pointer to the memory to deallocate
/// @param old_count Number of elements of type T originally allocated
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
#define alloc_delete(ptr, old_count, ...)                                                          \
  ((void)rk_overload(RK__alloc_DELETE, ptr, old_count, ##__VA_ARGS__))

/// @brief `T* alloc_new_aligned(T, size_t count, size_t align, Allocator alloc = alloc_ctx)` -
/// Allocates memory for an array of `count` elements of type T with specified alignment.
/// @param T         The type of elements to allocate
/// @param count     Number of elements to allocate
/// @param align     Desired alignment of the memory, must be a power of two
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
/// @return Pointer to allocated and aligned memory block, cast to `T*`.
#define alloc_new_aligned(T, count, align, ...)                                                    \
  ((T*)rk_overload(RK__alloc_ALIGNED_NEW, T, count, align, ##__VA_ARGS__))

/// @brief `T* alloc_renew_aligned(T* ptr, size_t old_count, size_t new_count, size_t align,
/// Allocator alloc = alloc_ctx)` - Resizes (reallocates) memory block to hold `new_count` elements
/// of the same type.
/// @param ptr       Pointer to the existing allocated memory
/// @param old_count Number of elements of type T previously allocated
/// @param new_count Number of elements of type T to allocate after resizing
/// @param align     Alignment of the memory; must match the original allocation
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
/// @return Pointer to reallocated and aligned memory block, cast to the same pointer type.
#define alloc_renew_aligned(ptr, old_count, new_count, align, ...)                                 \
  ((typeof(ptr))rk_overload(RK__alloc_ALIGNED_RENEW, ptr, old_count, new_count,                    \
                            align, ##__VA_ARGS__))

/// @brief `void alloc_delete_aligned(T* ptr, size_t old_count, size_t align, Allocator alloc =
/// alloc_ctx)` - Deallocates aligned memory.
/// @param ptr       Pointer to the memory to deallocate
/// @param old_count Number of elements of type T originally allocated
/// @param align     Alignment of the memory; must match the original allocation
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
#define alloc_delete_aligned(ptr, old_count, align, ...)                                           \
  ((void)rk_overload(RK__alloc_ALIGNED_DELETE, ptr, old_count, align, ##__VA_ARGS__))

/// @brief `void* malloc_allocate(size_t nbytes, size_t align)` - Allocates `nbytes` bytes of memory
/// with the specified alignment.
/// @note Zero-sized allocations are guaranteed to return a null pointer. Adjusts size to be a
/// multiple of alignment on some platforms.
/// @param nbytes Number of bytes to allocate
/// @param align  Desired alignment of the memory; must be a power of two
/// @return Pointer to allocated and aligned memory block, or `NULL` iff `nbytes` is zero.
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
#define malloc_allocate(nbytes, align) ((void*)RK__malloc_ALLOCATE(nbytes, align))

/// @brief `void* malloc_reallocate(void* ptr, size_t obytes, size_t nbytes, size_t align)` -
/// Resizes an aligned memory block from `obytes` to `nbytes` bytes.
/// @note On MSVC, calls `_aligned_realloc`. On other platforms, allocates a new block, copies, and
/// frees the old one (no in-place realloc available). For standard-aligned allocations prefer
/// `malloc_renew`; for over-aligned allocations this is required.
/// @param ptr    Pointer to the existing allocated memory
/// @param obytes Old size of the allocation in bytes
/// @param nbytes New size of the allocation in bytes
/// @param align  Alignment of the memory; must match the original allocation
/// @return Pointer to reallocated and aligned memory block.
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
#define malloc_reallocate(ptr, obytes, nbytes, align)                                              \
  ((void*)RK__malloc_REALLOCATE(ptr, obytes, nbytes, align))

/// @brief `void malloc_deallocate(void* ptr, size_t align)` - Deallocates an aligned memory block
/// previously allocated with `malloc_allocate` or `malloc_reallocate`.
/// @param ptr   Pointer to the memory to deallocate
/// @param align Alignment of the memory; must match the original allocation
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
#define malloc_deallocate(ptr, align)       ((void)RK__malloc_DEALLOCATE(ptr, align))

/// @brief `T* malloc_new(T, size_t count)` - Allocates memory for an array of `count` elements of
/// type `T` using `malloc`.
/// @param T     The type of elements to allocate
/// @param count Number of elements to allocate
/// @return Pointer to allocated memory block, cast to `T*`.
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
/// @note Zero-sized allocations are guaranteed to return a null pointer. Errors are handled via the
/// `RK_MALLOC_FAIL` macro that may be redefined by the user.
#define malloc_new(T, count)                ((T*)RK__malloc_NEW(T, count))

/// @brief `T* malloc_renew(T* ptr, size_t count)` - Resizes (reallocates) memory block to hold
/// `count` elements of the same type.
/// @note Passing `count == 0` frees the memory. Passing `ptr == NULL` is equivalent to calling
/// `malloc_new`.
/// @param ptr   Pointer to the existing allocated memory
/// @param count Number of elements of type T to allocate after resizing
/// @return Pointer to reallocated memory block, cast to the same pointer type
/// @warning Must not be used on pointers from over-aligned allocations
#define malloc_renew(ptr, count)            ((typeof(ptr))RK__malloc_RENEW(ptr, count))

/// @brief `void malloc_delete(T* ptr)` - Deallocates memory previously allocated with one of the
/// macros defined in this interface.
/// @param ptr Pointer to the memory to deallocate
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
#define malloc_delete(ptr)                  ((void)RK__malloc_DELETE(ptr))

/// @brief `T* malloc_new_aligned(T, size_t count, size_t align)` - Allocates memory for an array of
/// `count` elements of type `T` with specified alignment using malloc (or _aligned_malloc with
/// standard alignment on Msvc).
/// @note Zero-sized allocations are guaranteed to return a null pointer. Adjusts size to be a
/// multiple of alignment on some platforms.
/// @param T     The type of elements to allocate
/// @param count Number of elements to allocate
/// @param align Desired alignment of the memory, must be a power of two
/// @return Pointer to allocated memory block, cast to `T*`, or `NULL` iff `count` is zero.
#define malloc_new_aligned(T, count, align) ((T*)RK__malloc_ALIGNED_NEW(T, count, align))

/// @brief `T* malloc_renew_aligned(T* ptr, size_t old_count, size_t new_count, size_t align)` -
/// Resizes (reallocates) an aligned memory block to hold `new_count` elements of the same type.
/// @note On MSVC, calls `_aligned_realloc`. On other platforms, allocates a new block, copies, and
/// frees the old one (no in-place realloc available). For standard-aligned types prefer
/// `malloc_renew`; for over-aligned types this is required.
/// @param ptr       Pointer to the existing allocated memory
/// @param old_count Old number of elements of type T
/// @param new_count New number of elements of type T
/// @param align     Alignment of the memory; must be a power of two
/// @return Pointer to reallocated memory block. `NULL` iff `new_count` is zero.
#define malloc_renew_aligned(ptr, old_count, new_count, align)                                     \
  ((typeof(ptr))RK__malloc_ALIGNED_RENEW(ptr, old_count, new_count, align))

/// @brief `void malloc_delete_aligned(T* ptr)` - Deallocates memory previously allocated with
/// malloc_new_aligned or with malloc_new for an over-aligned type. On non-MSVC it's always
/// identical to malloc_delete; on MSVC it uses _aligned_free instead of free
/// @param ptr Pointer to the memory to deallocate
/// @attention Do **not** mix these macros defined here with regular `malloc`/`free` for the same
/// pointers.
#define malloc_delete_aligned(ptr) ((void)RK__malloc_ALIGNED_DELETE(ptr))

/// @brief Allocate memory using OS-backed page mapping (`mmap` / `VirtualAlloc`). The returned
/// memory is zero-initialized and page-aligned. Allocation failures invoke `RK_MMAP_FAIL`, which
/// aborts by default.
/// @param size Size in bytes. Rounded up to the next page boundary internally.
/// @note Passing 0 returns `NULL` without invoking the failure handler.
/// @return Pointer to the allocated memory.
static_fun void* page_alloc(size_t size);

/// @brief Reallocate memory previously allocated with `page_alloc()`. On Linux, uses `mremap`
/// (in-place when possible). On other POSIX platforms, allocates a new region, copies, and unmaps
/// the old one. On Windows, uses `VirtualAlloc` + copy + `VirtualFree`.
/// @param ptr Pointer to the existing block (or `NULL` to act like `page_alloc`)
/// @param old_size Current size in bytes
/// @param new_size New size in bytes (or 0 to act like `page_free`)
/// @return Pointer to the reallocated memory block.
static_fun void* page_realloc(void* ptr, size_t old_size, size_t new_size);

/// @brief Free memory allocated via `page_alloc()`.
/// @param ptr  Pointer to the memory block to free
/// @param size Size of the block being freed, in bytes (must match allocation)
/// @note Calling this with `size == 0` is a no-op.
static_fun void  page_free(void* ptr, size_t size);

/// @brief `T* rk_arrdup(T* src, size_t count, Allocator alloc = alloc_ctx)` - Copies an array of
/// objects from `src` onto allocated storage
/// @param src       The address of the array (must be typed correctly)
/// @param count     The count of objects to copy
/// @param allocator The Allocator to use (defaults to `alloc_ctx`)
/// @return A pointer to the allocated array
#define rk_arrdup(src, count, ...)                                                                 \
  ((typeof(((void)0, (src)[0]))*)rk_overload(RK__ARRDUP, src, count, ##__VA_ARGS__))

////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////Implementation Details///////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
/// @cond INTERNAL

#if RK_CUSTOM_ALLOCATORS
# define rk_assert_allocator_valid(_alloc) rk_assert((_alloc).vtab && "Invalid Allocator")
#else
# define rk_assert_allocator_valid(_alloc) ((void)0)
#endif

#if RK_CUSTOM_ALLOCATORS
# define RK_IFALLOC(...) __VA_ARGS__
# define rk_set_alloc_fallback(_alloc)                                                             \
   ((void)(rk_likely((_alloc).vtab)                                                                \
               ? alloc_ctx                                                                         \
               : (rk_assert_allocator_valid(alloc_ctx), (_alloc) = alloc_ctx)))
#else
# define RK_IFALLOC(...)
# define rk_set_alloc_fallback(_alloc) ((void)0)
#endif

///////////////////////// Page Allocator /////////////////////////////////
#if defined(_MSC_VER) && !defined(_WINDOWS_)
__declspec(dllimport) void* __stdcall VirtualAlloc(void* lpAddress, size_t dwSize,
                                                   unsigned long flAllocationType,
                                                   unsigned long flProtect);
__declspec(dllimport) int __stdcall   VirtualFree(void* lpAddress, size_t dwSize,
                                                  unsigned long dwFreeType);
#endif

static_fun size_t RK__mmap_page_size(void) {
#ifndef _MSC_VER
  long ps = sysconf(_SC_PAGESIZE);
  RK_MMAP_FAIL(ps != -1, ps, rk_null, 0, 0);
  return (size_t)ps;
#else
  return 4096;
#endif
}

static_fun rk_malloc_fun rk_alloc_size(1) void* page_alloc(size_t size) {
  if rk_unlikely (!size) { return rk_null; }
  size_t ps = RK__mmap_page_size();
  size      = rk_align_up(size, ps);
#ifndef _MSC_VER
  void* res = mmap(rk_null, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  RK_MMAP_FAIL(res != MAP_FAILED, ps, rk_null, 0, size);
#else
  void* res = VirtualAlloc(rk_null, size, 0x00001000 | 0x00002000, 0x04);
  RK_MMAP_FAIL(res, ps, rk_null, 0, size);
#endif
  return res;
}

static_fun void page_free(void* ptr, size_t size) {
  if rk_unlikely (!size) { return; }
  size_t ps = RK__mmap_page_size();
  size      = rk_align_up(size, ps);
#ifndef _MSC_VER
  int r = munmap(ptr, size);
  RK_MMAP_FAIL(r == 0, ps, ptr, 0, size);
#else
  int r = VirtualFree(ptr, 0, 0x00008000);
  RK_MMAP_FAIL(r != 0, ps, ptr, 0, size);
#endif
}

static_fun rk_alloc_size(3) void* page_realloc(void* ptr, size_t old_size, size_t new_size) {
  if (!old_size) { return page_alloc(new_size); }
  if (!new_size) { return page_free(ptr, old_size), rk_null; }
  size_t ps      = RK__mmap_page_size();
  size_t al_size = rk_align_up(new_size, ps), al_oldsize = rk_align_up(old_size, ps);
  if (al_size == al_oldsize) {
    return ptr;
  } else if (al_size < al_oldsize) {
#ifndef _MSC_VER
    int r = munmap((char*)ptr + al_size, al_oldsize - al_size);
    RK_MMAP_FAIL(r == 0, ps, ptr, 0, new_size);
#else
    int r = VirtualFree((char*)ptr + al_size, al_oldsize - al_size, 0x4000);
    RK_MMAP_FAIL(r != 0, ps, ptr, 0, new_size);
#endif
    return ptr;
  } else {
#if defined(__linux__) && defined(MREMAP_MAYMOVE)
    void* res = mremap(ptr, al_oldsize, al_size, MREMAP_MAYMOVE);
    RK_MMAP_FAIL(res != MAP_FAILED, ps, ptr, 0, new_size);
#elif !defined(_MSC_VER)
    void* res = mmap(rk_null, al_size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    RK_MMAP_FAIL(res != MAP_FAILED, ps, rk_null, 0, new_size);
    rk_memcpy(res, ptr, old_size);
    int r = munmap(ptr, al_oldsize);
    RK_MMAP_FAIL(r == 0, ps, ptr, 0, new_size);
#else
    void* res = VirtualAlloc(rk_null, al_size, 0x00001000 | 0x00002000, 0x04);
    RK_MMAP_FAIL(res, ps, rk_null, 0, new_size);
    rk_memcpy(res, ptr, old_size);
    int r = VirtualFree(ptr, 0, 0x00008000);
    RK_MMAP_FAIL(r != 0, ps, ptr, 0, new_size);
#endif
    return res;
  }
}

static_fun rk_malloc_fun rk_alloc_alignsize(2, 1) void* RK__page_allocate(size_t       size,
                                                                          size_t align rk_unused,
                                                                          void* ctx    rk_unused) {
  rk_assert(align <= RK__mmap_page_size() && "Wrong alignment");
  return page_alloc(size);
}

static_fun rk_alloc_alignsize(4, 3) void* RK__page_reallocate(void* ptr, size_t old_size,
                                                              size_t       new_size,
                                                              size_t align rk_unused,
                                                              void* ctx    rk_unused) {
  rk_assert(align <= RK__mmap_page_size() && "Wrong alignment");
  return page_realloc(ptr, old_size, new_size);
}

static_fun void RK__page_deallocate(void* ptr, size_t old_size, size_t align rk_unused,
                                    void* ctx rk_unused) {
  rk_assert(align <= RK__mmap_page_size() && "Wrong alignment");
  page_free(ptr, old_size);
}

///////////////////////////////////    Malloc wrappers   ///////////////////////////////////////////
static_fun rk_forceinline rk_malloc_fun rk_alloc_size(1) void* RK__malloc_f(size_t size) {
  if rk_unlikely (!size) { return rk_null; }
  void* res = malloc(size);
  RK_MALLOC_FAIL(res, rk_null, rk_null, RK_malloc_align, size);
  return res;
}

static_fun rk_forceinline void RK__free_f(void* ptr) {
  if rk_unlikely (ptr == rk_null) { return; }
  free(ptr);
}

static_fun rk_forceinline rk_alloc_size(2) void* RK__realloc_f(void* ptr, size_t size) {
  if (!size) { return RK__free_f(ptr), rk_null; }
  if (ptr == rk_null) { return RK__malloc_f(size); }
  void* res = realloc(ptr, size);
  RK_MALLOC_FAIL(res, rk_null, ptr, RK_malloc_align, size);
  return res;
}

static_fun rk_malloc_fun rk_alloc_alignsize(2, 1) void* RK__aligned_alloc_f(size_t size,
                                                                            size_t align) {
  rk_assert_align_pow2(align);
  if rk_unlikely (!size) { return rk_null; }
  align = rk_max(align, RK_malloc_align);
  size  = rk_align_up(size, align);
#ifndef _MSC_VER
  void* res = aligned_alloc(align, size);
#else
  void* res = _aligned_malloc(size, align);
#endif
  RK_MALLOC_FAIL(res, rk_null, rk_null, align, size);
  return res;
}

#ifndef _MSC_VER
# define RK__aligned_free_f RK__free_f
#else
static_fun rk_forceinline void RK__aligned_free_f(void* ptr) {
  if (ptr != rk_null) { _aligned_free(ptr); }
}
#endif

static_fun rk_forceinline rk_alloc_alignsize(4, 3) void* RK__aligned_realloc_f(void*  ptr,
                                                                               size_t old_size,
                                                                               size_t new_size,
                                                                               size_t align) {
  if (!old_size) { return RK__aligned_alloc_f(new_size, align); }
  if (!new_size) { return RK__aligned_free_f(ptr), rk_null; }
  rk_assert_align_pow2(align);
  align    = rk_max(align, RK_malloc_align);
  new_size = rk_align_up(new_size, align);
#ifndef _MSC_VER
  void* res = aligned_alloc(align, new_size);
  RK_MALLOC_FAIL(res, rk_null, ptr, align, new_size);
  memcpy(res, ptr, rk_min(new_size, old_size));
  free(ptr);
#else
  void* res = _aligned_realloc(ptr, new_size, align);
  RK_MALLOC_FAIL(res, rk_null, ptr, align, new_size);
  (void)old_size;
#endif
  return res;
}

static_fun rk_malloc_fun rk_alloc_alignsize(2, 1) void* RK__malloc_allocate(size_t    size,
                                                                            size_t    align,
                                                                            void* ctx rk_unused) {
  return align <= RK_malloc_align ? RK__malloc_f(size) : RK__aligned_alloc_f(size, align);
}

static_fun rk_alloc_alignsize(4, 3) void* RK__malloc_reallocate(void* ptr, size_t old_size,
                                                                size_t new_size, size_t align,
                                                                void* ctx rk_unused) {
  return align <= RK_malloc_align ? RK__realloc_f(ptr, new_size)
                                  : RK__aligned_realloc_f(ptr, old_size, new_size, align);
}

static_fun void RK__malloc_deallocate(void* ptr, size_t old_size rk_unused, size_t align rk_unused,
                                      void* ctx rk_unused) {
  align <= RK_malloc_align ? RK__free_f(ptr) : RK__aligned_free_f(ptr);
}

// dynamically chose whether malloc or aligned_alloc
#define RK__malloc_ALLOCATE(bytes, align)                                                          \
  (alloc_log_new, RK__malloc_allocate(bytes, align, rk_null))
#define RK__malloc_REALLOCATE(ptr, obytes, nbytes, align)                                          \
  (alloc_log_renew, RK__malloc_reallocate(ptr, obytes, nbytes, align, rk_null))
#define RK__malloc_DEALLOCATE(ptr, align)                                                          \
  (alloc_log_delete, RK__malloc_deallocate(ptr, 0, align, rk_null))

// always call malloc, compiler error if over-aligned
#define RK__malloc_NEW(T, count)                                                                   \
  (alloc_log_new, rk_ensure_malloc_align(T), RK__malloc_f(sizeof_n(T, count)))
#define RK__malloc_RENEW(ptr, count)                                                               \
  (alloc_log_renew, rk_ensure_malloc_align(typeof(*(ptr))),                                        \
   RK__realloc_f(ptr, sizeof_n(*(ptr), count)))
#define RK__malloc_DELETE(ptr)                                                                     \
  (alloc_log_delete, rk_ensure_malloc_align(typeof(*(ptr))), RK__free_f(ptr))

// always call aligned_alloc, check if alignment is enough for type
#define RK__malloc_ALIGNED_NEW(T, count, align)                                                    \
  (alloc_log_new, rk_assert_valid_align(T, align), RK__aligned_alloc_f(sizeof_n(T, count), align))
#define RK__malloc_ALIGNED_RENEW(ptr, old_count, new_count, align)                                 \
  (alloc_log_renew, rk_assert_valid_align(typeof(*(ptr)), align),                                  \
   RK__aligned_realloc_f(ptr, sizeof_n(*(ptr), old_count), sizeof_n(*(ptr), new_count), align))
#define RK__malloc_ALIGNED_DELETE(ptr) (alloc_log_delete, RK__aligned_free_f(ptr))

///////////////////////////////////  Alloc Wrappers ////////////////////////////////////////////////

#if RK_CUSTOM_ALLOCATORS
static_fun rk_forceinline rk_alloc_alignsize(2, 1) void* RK__call_alloc(size_t nbytes, size_t align,
                                                                        Allocator alloc) {
  rk_assert(alloc.vtab && "Invalid Allocator");
  if (!nbytes) { return rk_null; }
  return alloc.vtab->alloc_f(nbytes, align, alloc.ctx);
}
static_fun rk_forceinline rk_alloc_alignsize(4, 3) void* RK__call_realloc(void* ptr, size_t obytes,
                                                                          size_t    nbytes,
                                                                          size_t    align,
                                                                          Allocator alloc) {
  rk_assert(alloc.vtab && "Invalid Allocator");
  if (!nbytes) {
    if (ptr) {
      rk_assert(obytes && "Non-NULL allocation has zero size");
      alloc.vtab->dealloc_f(ptr, obytes, align, alloc.ctx);
    } else {
      rk_assert(!obytes && "NULL allocation has nonzero size");
    }
    return rk_null;
  }
  if (!ptr) {
    rk_assert(!obytes && "NULL allocation has nonzero size");
    return alloc.vtab->alloc_f(nbytes, align, alloc.ctx);
  }
  rk_assert(obytes && "Non-NULL allocation has zero size");
  return alloc.vtab->realloc_f(ptr, obytes, nbytes, align, alloc.ctx);
}

static_fun rk_forceinline void RK__call_dealloc(void* ptr, size_t obytes, size_t align,
                                                Allocator alloc) {
  rk_assert(alloc.vtab && "Invalid Allocator");
  if (!ptr) {
    rk_assert(!obytes && "NULL allocation has nonzero size");
    return;
  }
  rk_assert(obytes && "Non-NULL allocation has zero size");
  alloc.vtab->dealloc_f(ptr, obytes, align, alloc.ctx);
}
#endif

#if RK_CUSTOM_ALLOCATORS
# define RK__alloc_ALLOCATE(bytes, align, all) (alloc_log_new, RK__call_alloc(bytes, align, all))
# define RK__alloc_REALLOCATE(ptr, obytes, nbytes, align, all)                                     \
   (alloc_log_renew, RK__call_realloc(ptr, obytes, nbytes, align, all))
# define RK__alloc_DEALLOCATE(ptr, obytes, align, all)                                             \
   (alloc_log_delete, RK__call_dealloc(ptr, obytes, align, all))
#else
# define RK__alloc_ALLOCATE(bytes, align, all) RK__malloc_ALLOCATE(bytes, align)
# define RK__alloc_REALLOCATE(ptr, obytes, nbytes, align, all)                                     \
   RK__malloc_REALLOCATE(ptr, obytes, nbytes, align)
# define RK__alloc_DEALLOCATE(ptr, obytes, align, all)                                             \
   ((void)(obytes), RK__malloc_DEALLOCATE(ptr, align))

#endif

#define RK__alloc_NEW(T, count, all) RK__alloc_ALLOCATE(sizeof_n(T, count), alignof(T), all)
#define RK__alloc_ALIGNED_NEW(T, count, align, all)                                                \
  (rk_assert_valid_align(T, align), RK__alloc_ALLOCATE(sizeof_n(T, count), align, all))

#define RK__alloc_RENEW(ptr, ocount, ncount, all)                                                  \
  RK__alloc_REALLOCATE(ptr, sizeof_n(*(ptr), ocount), sizeof_n(*(ptr), ncount),                    \
                       alignof(typeof(*(ptr))), all)
#define RK__alloc_ALIGNED_RENEW(ptr, ocount, ncount, align, all)                                   \
  (rk_assert_valid_align(typeof(*(ptr)), align),                                                   \
   RK__alloc_REALLOCATE(ptr, sizeof_n(*(ptr), ocount), sizeof_n(*(ptr), ncount), align, all))

#define RK__alloc_DELETE(ptr, ocount, all)                                                         \
  RK__alloc_DEALLOCATE(ptr, sizeof_n(*(ptr), ocount), alignof(typeof(*(ptr))), all)
#define RK__alloc_ALIGNED_DELETE(ptr, ocount, align, all)                                          \
  (rk_assert_valid_align(typeof(*(ptr)), align),                                                   \
   RK__alloc_DEALLOCATE(ptr, sizeof_n(*(ptr), ocount), align, all))

// macros with allocator parameter; disabled if no local allocators enabled
#define RK__alloc_ALLOCATE3(bytes, align, all) rk_disable_if(RK__alloc_ALLOCATE(bytes, align, all))
#define RK__alloc_REALLOCATE5(ptr, obytes, nbytes, align, all)                                     \
  rk_disable_if(RK__alloc_REALLOCATE(ptr, obytes, nbytes, align, all))
#define RK__alloc_DEALLOCATE4(ptr, obytes, align, all)                                             \
  rk_disable_if(RK__alloc_DEALLOCATE(ptr, obytes, align, all))

#define RK__alloc_NEW3(T, count, all) rk_disable_if(RK__alloc_NEW(T, count, all))
#define RK__alloc_ALIGNED_NEW4(T, count, align, all)                                               \
  rk_disable_if(RK__alloc_ALIGNED_NEW(T, count, align, all))

#define RK__alloc_RENEW4(ptr, ocount, ncount, all)                                                 \
  rk_disable_if(RK__alloc_RENEW(ptr, ocount, ncount, all))
#define RK__alloc_ALIGNED_RENEW5(ptr, ocount, ncount, align, all)                                  \
  rk_disable_if(RK__alloc_ALIGNED_RENEW(ptr, ocount, ncount, align, all))

#define RK__alloc_DELETE3(ptr, ocount, all) rk_disable_if(RK__alloc_DELETE(ptr, ocount, all))
#define RK__alloc_ALIGNED_DELETE4(ptr, ocount, align, all)                                         \
  rk_disable_if(RK__alloc_ALIGNED_DELETE(ptr, ocount, align, all))
// get_alloc_ctx
//  macros with fewer parameters (might default to alloc_ctx)
#define RK__alloc_ALLOCATE2(bytes, align)       RK__alloc_ALLOCATE(bytes, align, alloc_ctx)
#define RK__alloc_ALIGNED_NEW3(T, count, align) RK__alloc_ALIGNED_NEW(T, count, align, alloc_ctx)
#define RK__alloc_NEW2(T, count)                RK__alloc_NEW(T, count, alloc_ctx)

#define RK__alloc_REALLOCATE4(ptr, obytes, nbytes, align)                                          \
  RK__alloc_REALLOCATE(ptr, obytes, nbytes, align, alloc_ctx)
#define RK__alloc_ALIGNED_RENEW4(ptr, ocount, ncount, align)                                       \
  RK__alloc_ALIGNED_RENEW(ptr, ocount, ncount, align, alloc_ctx)
#define RK__alloc_RENEW3(ptr, ocount, ncount) RK__alloc_RENEW(ptr, ocount, ncount, alloc_ctx)

#define RK__alloc_DEALLOCATE3(ptr, obytes, align)                                                  \
  RK__alloc_DEALLOCATE(ptr, obytes, align, alloc_ctx)
#define RK__alloc_ALIGNED_DELETE3(ptr, ocount, align)                                              \
  RK__alloc_ALIGNED_DELETE(ptr, ocount, align, alloc_ctx)
#define RK__alloc_DELETE2(ptr, ocount) RK__alloc_DELETE(ptr, ocount, alloc_ctx)

static_fun rk_alloc_alignsize(3, 2) void* RK__arrdup_f(const void* src, size_t size,
                                                       size_t align RK_IFALLOC(, Allocator alloc)) {
  return rk_memcpy(alloc_allocate(size, align RK_IFALLOC(, alloc)), src, size);
}
#define RK__ARRDUP(src, count, alloc)                                                              \
  RK__arrdup_f(src, sizeof_n(typeof(*(src)), count), alignof(typeof(*(src))) RK_IFALLOC(, alloc))
#define RK__ARRDUP3(src, count, alloc) rk_disable_if(RK__ARRDUP(src, count, alloc))
#define RK__ARRDUP2(src, count)        RK__ARRDUP(src, count, alloc_ctx)

#undef RK__ALLOCCTX_STORAGE
#undef RK__ALLOCCTX_INIT
/// @endcond

RK_HEADER_END

/// @}
#endif // RK_ALLOC_H

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
